/*
gcc -o smr -g -Wall smr.c
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./smr
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "smr_utilities.h"
#include "smr.h"

/* =============================================================================
UTILITY FUNCTIONS
============================================================================= */
double get_sample_interval(struct SMRFileHeader *fhdr, int idx)
{
    /*NOTE: currently sample interval is in MICROSECONDS*/
    double interval = -1.0;
    struct SMRChannelHeader *chdr = NULL;

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    switch (chdr->kind)
    {
        case 1:
        case 6:
        case 7:
        case 9:

            if (fhdr->system_id < 6)
            {
                interval = (double)chdr->divide * (double)fhdr->uspertime
                    * (double)fhdr->timeperadc;
            }
            else
            {
                interval = (double)chdr->l_chan_dvd * (double)fhdr->uspertime
                    * fhdr->dtimebase * 1e6;
            }

            break;

        default:

            fprintf(stderr, "ERROR: channel type has no sample interval!\n");
            interval = 0;
            break;
    }

cleanup:
    free_channel_header(chdr);

    return interval;
}
/* ========================================================================== */
int channel_label_to_index(struct SMRFileHeader *fhdr, const char *label)
{
    struct SMRChannelInfoArray *ifo = NULL;

    unsigned int idx = -1;
    unsigned int k = 0;

    if ((ifo = read_channel_info_array(fhdr)) == NULL)
    {
        goto cleanup;
    }

    while (idx == -1 && k < ifo->length)
    {
        /*NOTE: we are now using our own version of strcasecmp (strcmpi) from
          smr_utilities.h */
        if (string_compare_nocase(label, ifo->ifo[k]->title) == 0)
        {
            idx = ifo->ifo[k]->index;
        }

        ++k;
    }

cleanup:
    free_channel_info_array(ifo);

    return idx;
}
/* -------------------------------------------------------------------------- */
int channel_label_path_to_index(const char *ifile, const char *label)
{
    int idx = -1;
    struct SMRFileHeader *fhdr;

    if ((fhdr = read_file_header(ifile)) != NULL)
    {
        idx = channel_label_to_index(fhdr, label);
        free_file_header(fhdr);
    }

    return idx;
}
/* ========================================================================== */
double ticks_to_seconds(struct SMRFileHeader *fhdr, int32_t time)
{
    return (double)time * (double)fhdr->uspertime * fhdr->dtimebase;
}
/* =============================================================================
HEADER READ & FREE FUNCTIONS
============================================================================= */
struct SMRFileHeader *read_file_header(const char *ifile)
{
    unsigned int k;
    FILE *fp;
    struct SMRFileHeader *hdr = NULL;

    if ((fp = open_file(ifile, FILE_READ_MODE)) == NULL)
    {
        fprintf(stderr, "[ERROR]: failed to open file - %s\n", ifile);
        return NULL;
    }

    rewind(fp);

    hdr = malloc(sizeof (struct SMRFileHeader));

    hdr->filepath = copy_string(ifile);

    fread(&hdr->system_id, sizeof (hdr->system_id), 1, fp);

    read_chars(fp, "%10c", hdr->copyright, sizeof (hdr->copyright));
    hdr->copyright[10] = '\0';

    read_chars(fp, "%8c", hdr->creator, sizeof (hdr->creator));
    hdr->creator[8] = '\0';

    fread(&hdr->uspertime, sizeof (hdr->uspertime), 1, fp);
    fread(&hdr->timeperadc, sizeof (hdr->timeperadc), 1, fp);
    fread(&hdr->filestate, sizeof (hdr->filestate), 1, fp);

    fread(&hdr->firstdata, sizeof (hdr->firstdata), 1, fp);

    fread(&hdr->nchannel, sizeof (hdr->nchannel), 1, fp);
    fread(&hdr->chansize, sizeof (hdr->chansize), 1, fp);
    fread(&hdr->extra_data, sizeof (hdr->extra_data), 1, fp);
    fread(&hdr->buffersize, sizeof (hdr->buffersize), 1, fp);
    fread(&hdr->osformat, sizeof (hdr->osformat), 1, fp);

    fread(&hdr->maxtime, sizeof (hdr->maxtime), 1, fp);

    fread(&hdr->dtimebase, sizeof (hdr->dtimebase), 1, fp);

    fread(hdr->time_detail, sizeof (uint8_t), 6, fp);
    fread(&hdr->time_year, sizeof (hdr->time_year), 1, fp);

    fread(hdr->pad, sizeof (char), 52, fp);
    hdr->pad[52] = '\0';

    for (k = 0; k < 5; ++k)
    {
        hdr->comment[k] = fill_string(fp, 79);
    }

    fclose(fp);

    return hdr;
}
/* -------------------------------------------------------------------------- */
void free_file_header(struct SMRFileHeader *hdr)
{
    if (hdr)
    {
        unsigned int k;

        for (k = 0; k < 5; ++k)
        {
            if (hdr->comment[k]) { free(hdr->comment[k]); }
        }

        if (hdr->filepath) { free(hdr->filepath); }

        free(hdr);
    }
}
/* ========================================================================== */
struct SMRChannelHeader *read_channel_header(struct SMRFileHeader *hdr, int idx)
{
    struct SMRChannelHeader *chan = NULL;
    FILE * fp;

    if ((idx > (int)hdr->nchannel) | (idx < 1))
    {
        char msg[80];
        sprintf(msg, "Requested channel [%d] is out of range [%d]", idx, hdr->nchannel);
        fprintf(stderr, "ERROR: %s\n", msg);

        return NULL;
    }

    if ((fp = open_file(hdr->filepath, FILE_READ_MODE)) == NULL)
    {
        return NULL;
    }

    rewind(fp);

    chan = malloc(sizeof (struct SMRChannelHeader));

    chan->index = idx;

    chan->filepath = copy_string(hdr->filepath);

    /*offset for header and preceeding channels*/
    fseek(fp, 512+(140*(idx-1)), SEEK_SET);

    fread(&chan->del_size, sizeof (chan->del_size), 1, fp);

    fread(&chan->next_del_block, sizeof (chan->next_del_block), 1, fp);
    fread(&chan->first_block, sizeof (chan->first_block), 1, fp);
    fread(&chan->last_block, sizeof (chan->last_block), 1, fp);

    fread(&chan->nblock, sizeof (chan->nblock), 1, fp);
    fread(&chan->nextra, sizeof (chan->nextra), 1, fp);
    fread(&chan->pre_trig, sizeof (chan->pre_trig), 1, fp);
    fread(&chan->free_0, sizeof (chan->free_0), 1, fp);
    fread(&chan->phy_sz, sizeof (chan->phy_sz), 1, fp);
    fread(&chan->max_data, sizeof (chan->max_data), 1, fp);

    chan->comment = fill_string(fp, 71);

    fread(&chan->max_chan_time, sizeof (chan->max_chan_time), 1, fp);
    fread(&chan->l_chan_dvd, sizeof (chan->l_chan_dvd), 1, fp);

    fread(&chan->phy_chan, sizeof (chan->phy_chan), 1, fp);

    chan->title = fill_string(fp, 9);

    fread(&chan->ideal_rate, sizeof (chan->ideal_rate), 1, fp);
    fread(&chan->kind, sizeof (chan->kind), 1, fp);
    fread(&chan->pad, sizeof (chan->pad), 1, fp);

    chan->units = NULL;

    chan->divide = 1;
    chan->interleave = 1;

    chan->scale = 0.0;
    chan->offset = 0.0;
    chan->min = 0.0;
    chan->max = 0.0;
    chan->init_low = 0;
    chan->next_low = 0;

    switch(chan->kind)
    {
        case CONTINUOUS_CHANNEL:
        case ADC_MARKER_CHANNEL:

            fread(&chan->scale, sizeof (chan->scale), 1, fp);
            fread(&chan->offset, sizeof (chan->offset), 1, fp);

            chan->units = fill_string(fp, 5);

            if (hdr->system_id < 6)
            {
                fread(&chan->divide, sizeof (chan->divide), 1, fp);
            }
            else
            {
                fread(&chan->interleave, sizeof (chan->interleave), 1, fp);
            }

            break;

        case REAL_MARKER_CHANNEL:
        case REAL_WAVE_CHANNEL:

            fread(&chan->min, sizeof (chan->min), 1, fp);
            fread(&chan->max, sizeof (chan->max), 1, fp);

            chan->units = fill_string(fp, 5);

            if (hdr->system_id < 6)
            {
                fread(&chan->divide, sizeof (chan->divide), 1, fp);
            }
            else
            {
                fread(&chan->interleave, sizeof (chan->interleave), 1, fp);
            }

            break;

        case EVENT_4_CHANNEL:

            fread(&chan->init_low, 1, sizeof (chan->init_low), fp);
            fread(&chan->next_low, 1, sizeof (chan->next_low), fp);

            break;
    }

    fclose(fp);

    return chan;
}
/* -------------------------------------------------------------------------- */
void free_channel_header(struct SMRChannelHeader *chan)
{
    if (chan)
    {
        if (chan->filepath) { free(chan->filepath); }

        if (chan->comment) { free(chan->comment); }

        if (chan->title) { free(chan->title); }

        if (chan->units) { free(chan->units); }

        free(chan);
    }
}
/* ========================================================================== */
struct SMRBlockHeaderArray *read_block_header_array(struct SMRChannelHeader *chan)
{
    FILE *fp;
    unsigned int k;
    struct SMRBlockHeaderArray *hdr_array = NULL;

    if ((int)chan->first_block == -1)
    {
        char msg[80];
        sprintf(msg, "Channel [%d - %s] contains no data", chan->index, chan->title);
        fprintf(stderr, "ERROR: %s\n", msg);

        return NULL;
    }

    if ((fp = open_file(chan->filepath, FILE_READ_MODE)) == NULL)
    {
        return NULL;
    }

    hdr_array = malloc(sizeof (struct SMRBlockHeaderArray));
    hdr_array->hdr = malloc(sizeof (struct SMRBlockHeader) * chan->nblock);

    fseek(fp, chan->first_block, SEEK_SET);

    hdr_array->hdr[0] = read_block_header(fp);

    if (hdr_array->hdr[0].last_block == -1)
    {
        hdr_array->hdr[0].next_block = chan->first_block;
        hdr_array->length = 1;

    }
    else
    {
        fseek(fp, hdr_array->hdr[0].last_block, SEEK_SET);
        hdr_array->length = chan->nblock;

        for (k = 1; k < chan->nblock; ++k)
        {
            hdr_array->hdr[k] = read_block_header(fp);

            fseek(fp, hdr_array->hdr[k].last_block, SEEK_SET);

            hdr_array->hdr[k-1].next_block = hdr_array->hdr[k].next_block;
        }

        hdr_array->hdr[chan->nblock-1].next_block = hdr_array->hdr[chan->nblock-2].last_block;
        hdr_array->length = chan->nblock;
    }

    fclose(fp);

    return hdr_array;
}
/* -------------------------------------------------------------------------- */
struct SMRBlockHeader read_block_header(FILE *fp)
{
    struct SMRBlockHeader hdr;

    hdr.nitem = 0;

    fread(&hdr.next_block, sizeof (hdr.next_block), 1, fp);
    fread(&hdr.last_block, sizeof (hdr.last_block), 1, fp);
    fread(&hdr.start_time, sizeof (hdr.start_time), 1, fp);
    fread(&hdr.end_time, sizeof (hdr.end_time), 1, fp);

    fread(&hdr.index, sizeof (hdr.index), 1, fp);
    fread(&hdr.nitem, sizeof (hdr.nitem), 1, fp);

    return hdr;
}
/* -------------------------------------------------------------------------- */
void free_block_header_array(struct SMRBlockHeaderArray *ary)
{
    if (ary)
    {
        if (ary->hdr) { free(ary->hdr); }

        free(ary);
    }
}
/* ========================================================================== */
struct SMRChannelInfoArray *read_channel_info_array(struct SMRFileHeader *fhdr)
{
    struct SMRChannelInfoArray *ifo_array = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRChannelInfo **tmp = NULL;

    unsigned int total = 0;
    unsigned int inc = 0;
    unsigned int k;

    ifo_array = malloc(sizeof (struct SMRChannelInfoArray));

    /*temporary array of pointers to channel_info structs*/
    tmp = malloc(sizeof (struct SMRChannelInfo *) * fhdr->nchannel);

    for (k = 0; k < fhdr->nchannel; ++k)
    {
        /*channel numbering starts at 1, thus k+1*/
        if ((chdr = read_channel_header(fhdr, k+1)) == NULL)
        {
            /*failed to read channel header, skip this channel*/
            printf("WARNING: failed to read header for channel %d\n", k+1);
            tmp[k] = NULL;
        }
        else
        {

            tmp[k] = load_channel_info(chdr);

            if ((tmp[k] != NULL) && (tmp[k]->kind > 0))
            {
                ++total;
            }
        }

        free_channel_header(chdr);
    }

    ifo_array->ifo = malloc(sizeof (struct SMRChannelInfo *) * total);
    ifo_array->length = total;

    for (k = 0; k < fhdr->nchannel; ++k)
    {
        /*make sure allocation succeded*/
        if (tmp[k])
        {
            if (tmp[k]->kind > 0)
            {
                /*no need to copy the memory, just move the pointer to the
                  new array*/
                ifo_array->ifo[inc] = tmp[k];
                ++inc;
            }
            else
            {
                /*each channel_info struct is individually allocated, thus we
                  can free any unused (kind==0) structs*/
                free_channel_info(tmp[k]);
            }
        }
    }

    /*tmp is just an array of pointers, we have a copy of valid pointers from
      the array in ifo_array->ifo, thus we can free tmp*/
    free(tmp);

    return ifo_array;
}
/* -------------------------------------------------------------------------- */
struct SMRChannelInfo *load_channel_info(struct SMRChannelHeader *s)
{
    struct SMRChannelInfo *ifo = NULL;

    if (s->title != NULL)
    {
        /*make sure the given channel header is valid*/
        ifo = malloc(sizeof (struct SMRChannelInfo));

        ifo->title = copy_string(s->title);

        ifo->index = s->index;
        ifo->kind = s->kind;
        ifo->phy_chan = s->phy_chan;
    }

    return ifo;
}
/* -------------------------------------------------------------------------- */
void free_channel_info(struct SMRChannelInfo *ifo) {

    if (ifo)
    {
        if (ifo->title) { free(ifo->title); }

        free(ifo);
    }
}
/* -------------------------------------------------------------------------- */
void free_channel_info_array(struct SMRChannelInfoArray *s)
{
    if (s)
    {
        unsigned int k;

        for (k = 0; k < s->length; ++k)
        {
            free_channel_info(s->ifo[k]);
        }

        if (s->ifo) { free(s->ifo); }

        free(s);
    }
}
/* =============================================================================
CHANNEL READ & FREE FUNCTIONS
============================================================================= */
struct SMRWMrkChannel *read_wavemark_channel(const char *ifile, int idx)
{
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRBlockHeaderArray *bhdr = NULL;
    struct SMRWMrkChannel *chan = NULL;

    FILE *fp;

    uint64_t inc = 0;
    uint64_t k;
    uint64_t j;
    int32_t buf;

    if (idx < 0)
    {
        printf("[ERROR]: channel index '%d' is not a valid index\n", idx);
        goto cleanup;
    }

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    if (chdr->kind != ADC_MARKER_CHANNEL)
    {
        char msg[80];
        sprintf(msg, "Channel [%d - %s] is not a wavemark channel", chdr->index, chdr->title);
        fprintf(stderr, "ERROR: %s\n", msg);

        goto cleanup;
    }

    if ((bhdr = read_block_header_array(chdr)) == NULL)
    {
        goto cleanup;
    }

    chan = malloc(sizeof (struct SMRWMrkChannel));
    chan->length = 0;

    for (k = 0; k < bhdr->length; ++k)
    {
        chan->length += (uint64_t) bhdr->hdr[k].nitem;
    }

    /*data points per spike*/
    chan->npt = chdr->nextra / sizeof (int16_t);

    chan->timestamps = malloc(sizeof (double) * chan->length);
    chan->markers = malloc(sizeof (uint8_t) * chan->length * MARKER_SIZE);
    chan->wavemarks = malloc(sizeof (int16_t) * chan->length * chan->npt);

    if ((fp = open_file(ifile, FILE_READ_MODE)) == NULL)
    {
        free_wavemark_channel(chan);
        goto cleanup;
    }

    for (k = 0; k < chdr->nblock; ++k)
    {
        fseek(fp, bhdr->hdr[k].next_block + BLOCK_HEADER_SIZE, SEEK_SET);

        for (j = 0; j < bhdr->hdr[k].nitem; ++j, ++inc)
        {
            fread(&buf, sizeof (buf), 1, fp);
            fread(chan->markers+(inc*MARKER_SIZE), sizeof (uint8_t), MARKER_SIZE, fp);
            fread(chan->wavemarks+(inc*chan->npt), sizeof (int16_t), chan->npt, fp);

            /*convert time in ticks to seconds*/
            chan->timestamps[inc] = ticks_to_seconds(fhdr, buf);
        }
    }

    fclose(fp);

cleanup:
    free_channel_header(chdr);
    free_file_header(fhdr);
    free_block_header_array(bhdr);

    return chan;
}
/* -------------------------------------------------------------------------- */
void free_wavemark_channel(struct SMRWMrkChannel *chan)
{
    if (chan)
    {
        if (chan->timestamps) { free(chan->timestamps); }

        if (chan->markers) { free(chan->markers); }

        if (chan->wavemarks) { free(chan->wavemarks); }

        free(chan);
    }
}
/* ========================================================================== */
struct SMRContChannel *read_continuous_channel(const char *ifile, int idx)
{

    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRContChannel *chan = NULL;

    if (idx < 0)
    {
        printf("[ERROR]: channel index '%d' is not a valid index\n", idx);
        goto cleanup;
    }

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    if (chdr->kind != CONTINUOUS_CHANNEL)
    {
        char msg[80];
        sprintf(msg, "Channel [%d - %s] is not a continuous channel", chdr->index, chdr->title);
        fprintf(stderr, "ERROR: %s\n", msg);

        goto cleanup;
    }

    chan = read_continuous_channel_from_header(fhdr, chdr);

cleanup:
    free_channel_header(chdr);
    free_file_header(fhdr);

    return chan;
}
/* ========================================================================== */
struct SMRContChannel *read_continuous_channel_from_header(
    struct SMRFileHeader *fhdr, struct SMRChannelHeader *chdr)
{
    struct SMRBlockHeaderArray *bhdr = NULL;
    struct SMRContChannel *chan = NULL;

    FILE *fp;

    double sample_interval;
    uint64_t nframe = 1;
    uint64_t k;
    int32_t tmp;

    sample_interval = get_sample_interval(fhdr, chdr->index);

    if ((bhdr = read_block_header_array(chdr)) == NULL)
    {
        goto cleanup;
    }

    /* count the number of frames? */
    for (k = 0; k < bhdr->length-1; ++k)
    {
        tmp = bhdr->hdr[k+1].start_time - bhdr->hdr[k].end_time;

        if ((double)tmp > sample_interval) { ++nframe; }
    }

    fp = open_file(fhdr->filepath, FILE_READ_MODE);

    if (fp == NULL)
    {
        fprintf(stderr, "ERROR: failed to open file - %s\n", fhdr->filepath);
        goto cleanup;
    }

    rewind(fp);

    chan = malloc(sizeof (struct SMRContChannel));

    chan->sampling_rate = MICROSECONDS / sample_interval;

    if (nframe == 1)
    {
        /*NOTE: indicates continuous sampling */
        uint64_t nsample = 0;

        size_t ptr = 0;

        for (k = 0; k < bhdr->length; ++k)
        {
            /*get total number of samples in the record*/
            nsample += (uint64_t) bhdr->hdr[k].nitem;
        }

        chan->data = malloc(sizeof (int16_t) * nsample);

        for (k = 0; k < bhdr->length; ++k)
        {
            fseek(fp, bhdr->hdr[k].next_block + BLOCK_HEADER_SIZE, SEEK_SET);

            /*FIXME: this pointer opperation may not be kosher...*/
            if ((ptr + (size_t) bhdr->hdr[k].nitem) <= (size_t) nsample)
            {
                ptr  += fread(chan->data + ptr, sizeof (int16_t), bhdr->hdr[k].nitem, fp);
            }
            else
            {
                fprintf(stderr, "WARNING: write extends beyond allocated area\n");
                fclose(fp);
                free(chan->data);

                goto cleanup;
            }
        }
        chan->length = nsample;
    }
    else
    {
        /*triggered sampling... what is this?*/
        fprintf(stderr, "ERROR: triggered sampling is not yet supported!\n");
        fclose(fp);

        free_continuous_channel(chan);
        chan = NULL;

        goto cleanup;
    }

    fclose(fp);

cleanup:
    free_block_header_array(bhdr);

    return chan;
}
/* -------------------------------------------------------------------------- */
void free_continuous_channel(struct SMRContChannel *s)
{
    if (s)
    {
        if (s->data) { free(s->data); }

        free(s);
    }
}
/* ========================================================================== */
struct SMREventChannel *read_event_channel(const char *ifile, int idx)
{
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRBlockHeaderArray *bhdr = NULL;
    struct SMREventChannel *evt = NULL;

    FILE *fp;

    uint64_t nitem = 0ul;
    uint64_t k;

    int32_t *buffer = NULL;
    size_t ptr = 0;

    if (idx < 0)
    {
        printf("[ERROR]: channel index '%d' is not a valid index\n", idx);
        goto cleanup;
    }

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    if (chdr->kind < EVENT_2_CHANNEL || chdr->kind > EVENT_4_CHANNEL)
    {
        char msg[80];
        sprintf(msg, "Channel [%d - %s] is not an event channel", chdr->index, chdr->title);
        fprintf(stderr, "ERROR: %s\n", msg);

        goto cleanup;
    }

    if ((bhdr = read_block_header_array(chdr)) == NULL)
    {
        goto cleanup;
    }

    for (k = 0; k < bhdr->length; ++k)
    {
        /*get total number of samples in the record*/
        nitem += (uint64_t) bhdr->hdr[k].nitem;
    }

    if ((fp = open_file(ifile, FILE_READ_MODE)) == NULL)
    {
        fprintf(stderr, "ERROR: failed to open file - %s\n", fhdr->filepath);
        goto cleanup;
    }

    rewind(fp);

    evt = malloc(sizeof (struct SMREventChannel));
    buffer = malloc(sizeof (int32_t) * nitem);

    for (k = 0; k < bhdr->length; ++k)
    {
        fseek(fp, bhdr->hdr[k].next_block + BLOCK_HEADER_SIZE, SEEK_SET);

        /*FIXME: this pointer opperation may not be kosher...*/
        if ((ptr + (size_t) bhdr->hdr[k].nitem) <= (size_t) nitem)
        {
            ptr  += fread(buffer + ptr, sizeof (int32_t), bhdr->hdr[k].nitem, fp);
        }
        else
        {
            fprintf(stderr, "WARNING: write extends beyond allocated area\n");
            fclose(fp);

            free_event_channel(evt);
            evt = NULL;

            goto cleanup;
        }
    }

    evt->data = malloc(sizeof (double) * nitem);

    for (k = 0; k < nitem; ++k)
    {
        evt->data[k] = ticks_to_seconds(fhdr, buffer[k]);
    }

    evt->length = nitem;

    fclose(fp);

cleanup:
    if (buffer) { free(buffer); }
    free_block_header_array(bhdr);
    free_channel_header(chdr);
    free_file_header(fhdr);

    return evt;
}
/* -------------------------------------------------------------------------- */
void free_event_channel(struct SMREventChannel *s)
{
    if (s)
    {
        if (s->data) { free(s->data); }

        free(s);
    }
}
/* ========================================================================== */
struct SMRMarkerChannel *read_marker_channel(const char *ifile, int idx)
{
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRBlockHeaderArray *bhdr = NULL;
    struct SMRMarkerChannel *evt = NULL;

    FILE *fp;

    uint64_t inc = 0;
    uint64_t k;
    uint64_t j;
    uint8_t hastext;
    int32_t buf;

    if (idx < 0)
    {
        printf("[ERROR]: channel index '%d' is not a valid index\n", idx);
        goto cleanup;
    }

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    if (chdr->kind != MARKER_CHANNEL && chdr->kind != TEXT_MARKER_CHANNEL)
    {
        char msg[80];
        sprintf(msg, "Channel [%d - %s] is not an event marker channel", chdr->index, chdr->title);
        fprintf(stderr, "ERROR: %s\n", msg);

        goto cleanup;
    }

    if (chdr->kind == MARKER_CHANNEL)
    {
        hastext = 0;
    }
    else
    {
        hastext = 1;
    }

    if ((bhdr = read_block_header_array(chdr)) == NULL)
    {
        goto cleanup;
    }

    evt = malloc(sizeof (struct SMRMarkerChannel));


    if ((fp = open_file(ifile, FILE_READ_MODE)) == NULL)
    {
        fprintf(stderr, "ERROR: failed to open file - %s\n", ifile);
        goto cleanup;
    }

    rewind(fp);

    evt->length = 0;

    for (k = 0; k < bhdr->length; ++k)
    {
        evt->length += (uint64_t) bhdr->hdr[k].nitem;
    }

    /*number of characters in text field for each event*/
    if (hastext)
    {
        evt->npt = chdr->nextra / sizeof (uint8_t);
    }
    else
    {
        evt->npt = 1;
    }

    evt->timestamps = malloc(sizeof (double) * evt->length);
    evt->markers = malloc(sizeof (uint8_t) * evt->length * MARKER_SIZE);
    evt->text = malloc(sizeof (uint8_t) * evt->length * evt->npt);

    for (k = 0; k < chdr->nblock; ++k)
    {
        fseek(fp, bhdr->hdr[k].next_block + BLOCK_HEADER_SIZE, SEEK_SET);

        for (j = 0; j < bhdr->hdr[k].nitem; ++j, ++inc)
        {
            fread(&buf, sizeof (int32_t), 1, fp);
            fread(evt->markers+(inc*MARKER_SIZE), sizeof (uint8_t), MARKER_SIZE, fp);

            if (hastext)
            {
                fread(evt->text+(inc*evt->npt), sizeof (uint8_t), evt->npt, fp);
            }
            else
            {
                evt->text[inc] = 0;
            }

            /*convert time in ticks to seconds*/
            evt->timestamps[inc] = ticks_to_seconds(fhdr, buf);
        }
    }

    fclose(fp);

cleanup:
    free_block_header_array(bhdr);
    free_channel_header(chdr);
    free_file_header(fhdr);

    return evt;
}
/* -------------------------------------------------------------------------- */
void free_marker_channel(struct SMRMarkerChannel *s)
{
    if (s)
    {
        if (s->timestamps) { free(s->timestamps); }

        if (s->markers) { free(s->markers); }

        if (s->text) { free(s->text); }

        free(s);
    }
}
/* ========================================================================== */
/*this just provides a convienent interface for julia so a user can just
  pass a file path directly
*/
struct SMRChannelInfoArray *read_channel_array(const char *ifile)
{
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelInfoArray *ifo = NULL;

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    ifo = read_channel_info_array(fhdr);

cleanup:
    free_file_header(fhdr);

    return ifo;
}
/* ========================================================================== */
