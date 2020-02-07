#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "smr.h"

#define SUCCESS 0
#define pow2(y) ((uint8_t) pow(2.0, (double)y))

#define get_str(buf, x) (sprintf(buf, "%s", #x))

enum error_type_t
{
    READ_FILE_HEADER = 0,
    READ_CHANNEL_INFO_ARRAY = 1,
    READ_CHANNEL_HEADER = 2,
    READ_BLOCK_HEADER_ARRAY = 3,
    READ_WAVEMARK_CHANNEL = 4,
    READ_CONTINUOUS_CHANNEL = 5,
    READ_MARKER_CHANNEL = 6,
    READ_EVENT_CHANNEL = 7
};

/* ========================================================================= */
char *error_string(unsigned int x)
{
    uint8_t n = 40;
    char *str = malloc(sizeof (char) * n);

    switch (x) {
        case READ_FILE_HEADER:
            get_str(str, READ_FILE_HEADER);
            break;
        case READ_CHANNEL_INFO_ARRAY:
            get_str(str, READ_CHANNEL_INFO_ARRAY);
            break;
        case READ_CHANNEL_HEADER:
            get_str(str, READ_CHANNEL_HEADER);
            break;
        case READ_BLOCK_HEADER_ARRAY:
            get_str(str, READ_BLOCK_HEADER_ARRAY);
            break;
        case READ_WAVEMARK_CHANNEL:
            get_str(str, READ_WAVEMARK_CHANNEL);
            break;
        case READ_CONTINUOUS_CHANNEL:
            get_str(str, READ_CONTINUOUS_CHANNEL);
            break;
        case READ_MARKER_CHANNEL:
            get_str(str, READ_MARKER_CHANNEL);
            break;
        case READ_EVENT_CHANNEL:
            get_str(str, READ_EVENT_CHANNEL);
            break;
    }
    str[n-1] = '\0';

    return str;
}

/* ========================================================================= */

uint8_t test_all()
{
    const char *ifile = "./b1_con_006.smr";
    // const char *ifile = "/media/scottie/KINGSTON/libsmr/b1_con_006.smr";

    const char *wmrk_label = "WMrk 4";
    const char *cont_label = "Cont 5";
    const char *mrk_label = "PseudoTex";
    const char *evt_label = "Stim";

    int idx;

    uint8_t out = SUCCESS;

    struct SMRFileHeader *fhdr = read_file_header(ifile);

    if (fhdr)
    {
        printf("read_file_header: success\n");
    }
    else
    {
        return out | pow2(READ_FILE_HEADER);
    }

    struct SMRChannelInfoArray *ifo = read_channel_info_array(fhdr);

    if (ifo)
    {
        printf("read_channel_info_array: success\n");
    }
    else
    {
        free_file_header(fhdr);
        return out | pow2(READ_CHANNEL_INFO_ARRAY);
    }

    free_channel_info_array(ifo);

    idx = channel_label_to_index(fhdr, wmrk_label);

    struct SMRChannelHeader *chdr = read_channel_header(fhdr, idx);

    if (chdr)
    {
        printf("read_channel_header: success\n");
    }
    else
    {
        free_file_header(fhdr);
        return out | pow2(READ_CHANNEL_HEADER);
    }

    unsigned long int si = get_sample_interval(fhdr, idx);
    printf("\tsample interval of channel %d = %lu\n", idx, si);

    struct SMRBlockHeaderArray *bhdr = read_block_header_array(chdr);

    if (bhdr)
    {
        printf("read_block_header_array: success\n");
    }
    else
    {
        free_channel_header(chdr);
        free_file_header(fhdr);
        return out | pow2(READ_BLOCK_HEADER_ARRAY);
    }

    free_block_header_array(bhdr);
    free_channel_header(chdr);

    struct SMRWMrkChannel *wmrk = read_wavemark_channel(ifile, idx);

    if (wmrk)
    {
        printf("read_wavemark_channel: success\n");
        printf("\tnumber of spikes: %02lu\n", wmrk->length);
        printf("\ttimepoints-per-spike: %02lu\n", wmrk->npt);
    }
    else
    {
        out |= pow2(READ_WAVEMARK_CHANNEL);
    }

    free_wavemark_channel(wmrk);

    idx = channel_label_to_index(fhdr, cont_label);

    struct SMRContChannel *cont = read_continuous_channel(ifile, idx);

    if (cont)
    {
        printf("read_continuous_channel: success\n");
        printf("\tchannel length: %02lu\n", cont->length);
    }
    else
    {
        out |= pow2(READ_CONTINUOUS_CHANNEL);
    }

    free_continuous_channel(cont);

    idx = channel_label_to_index(fhdr, mrk_label);

    struct SMRMarkerChannel *mrk = read_marker_channel(ifile, idx);

    if (mrk)
    {
        printf("read_marker_channel: success\n");
    }
    else
    {
        out |= pow2(READ_MARKER_CHANNEL);
    }

    free_marker_channel(mrk);

    idx = channel_label_to_index(fhdr, evt_label);

    struct SMREventChannel *evt = read_event_channel(ifile, idx);

    if (evt)
    {
        printf("read_event_channel: success\n");
        printf("\t# of events = %lu\n", evt->length);
    }
    else
    {
        out |= pow2(READ_EVENT_CHANNEL);
    }

    free_event_channel(evt);

    // struct SMREventChannel *evt2 = read_event_channel(ifile, idx);
    //
    // if (evt2) {
    //     printf("read_event_channel [#2]: success\n");
    //     printf("\t# of events = %lu\n", evt2->length);
    //     size_t k;
    //     for (k = 0; k < evt2->length; ++k) {
    //         printf("\t%03d => %.2f\n", k, evt2->data[k]);
    //     }
    // } else {
    //     printf("read_event_channel [#2]: failure\n");
    // }
    //
    // free_event_channel(evt2);

    free_file_header(fhdr);

    return out;
}
/* ========================================================================= */

int main()
{
    unsigned int k;
    char *str;

    uint8_t result = test_all();

    for (k = 0; k < 8; ++k)
    {
        if (result >> k & 1)
        {
            str = error_string(k);
            if (str)
            {
                printf("********\n");
                printf("FAILURE: %s\n",str);
                printf("********\n");
                free(str);
            }
            else
            {
                fprintf(stderr, "Failed to get error string\n");
                return -1;
            }
        }
    }
    return 0;
}
/* ========================================================================= */
/*
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./smr_test
*/
