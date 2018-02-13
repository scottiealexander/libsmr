#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "smr.h"
// #include "smr_utilities.h"

/* -------------------------------------------------------------------------- */
#define show_error(msg) (fprintf(stderr, "[ERROR]: %s\n", msg))
#define FILE_WRITE_MODE "wb"

FILE *open_file(const char*, const char*);
/* -------------------------------------------------------------------------- */
typedef struct IntArray
{
    int *data;
    size_t length;
} IntArray;
/* -------------------------------------------------------------------------- */
void free_int_array(IntArray *ary)
{
    if (ary)
    {
        if (ary->data) { free(ary->data); }

        free(ary);
    }
}
/* -------------------------------------------------------------------------- */
char *swap_ext(const char *filepath, const char *ext)
{
    size_t len = strlen(filepath);
    size_t ptr = len - 1;

    while ((filepath[ptr] != '.') && (ptr > 0)) { --ptr; }
    size_t path_len = ptr > 0 ? ptr : len;

    size_t ext_len = strlen(ext);

    uint8_t pad = (ext[0] != '.') ? 1 : 0;

    char *out = malloc(path_len + ext_len + pad + 1);
    ptr = 0;

    memcpy(out, filepath, path_len);
    ptr += path_len;

    if (ext[0] != '.')
    {
        memcpy(out + ptr, ".", 1);
        ++ptr;
    }
    memcpy(out + ptr, ext, ext_len);

    out[path_len + ext_len + pad] = '\0';

    return out;
}
/* -------------------------------------------------------------------------- */
void dump_channel_info(const char* smrfile)
{
    struct SMRChannelInfoArray *ifo_array = read_channel_array(smrfile);
    if (ifo_array != NULL)
    {
        for (int32_t k = 0; k < ifo_array->length; ++k)
        {
            printf("%s", ifo_array->ifo[k]->title);
            for (uint8_t j = 0; j < 9 - strlen(ifo_array->ifo[k]->title); ++j)
            {
                printf(" ");
            }
            printf("| index: %d,  kind: %d, port: %d\n",
                ifo_array->ifo[k]->index,
                ifo_array->ifo[k]->kind,
                ifo_array->ifo[k]->phy_chan
            );
        }
        free_channel_info_array(ifo_array);
    }
}
/* -------------------------------------------------------------------------- */
IntArray *get_continuous_indicies(const char *smrfile)
{
    struct SMRChannelInfoArray *ifo_array = read_channel_array(smrfile);
    IntArray *out = NULL;

    if (ifo_array != NULL)
    {
        out = malloc(sizeof(IntArray));
        out->data = malloc(ifo_array->length * sizeof(int));

        int32_t ncont = 0;
        for (int32_t k = 0; k < ifo_array->length; ++k)
        {
            if (ifo_array->ifo[k]->kind == CONTINUOUS_CHANNEL)
            {
                out->data[ncont] = ifo_array->ifo[k]->index;
                ++ncont;
            }
        }

        free_channel_info_array(ifo_array);

        out->length = ncont;
    }

    return out;
}
/* -------------------------------------------------------------------------- */
size_t get_list_length(const char *list)
{
    size_t nchar = strlen(list);

    if (nchar > 0)
    {
        size_t nel = 1;
        for (size_t k = 0; k < nchar; ++k)
        {
            // don't count trailing ','
            if ((list[k] == ',') && (k < nchar - 1))
            {
                ++nel;
            }
        }
        return nel;
    }
    else
    {
        return 0;
    }

}
/* -------------------------------------------------------------------------- */
IntArray *get_indicies_from_list(const char *list)
{
    IntArray *out = malloc(sizeof(IntArray));

    size_t nel = get_list_length(list);

    out->data = malloc(nel * sizeof(int));
    out->length = nel;

    out->data[0] = atoi(list);
    size_t len = strlen(list);

    size_t ptr = 1;
    for (size_t k = 1; k < nel; ++k)
    {
        while ((list[ptr] != ',') && (ptr < len))
        {
            ++ptr;
        }

        // pointer should be one char past the last ','
        ++ptr;

        out->data[k] = atoi(list + ptr);
    }

    return out;
}
/* -------------------------------------------------------------------------- */
void write_mda_header(FILE *fp, int32_t nchan, int32_t npt)
{
    const int32_t type = -4; // int16 is -4 in mda
    const int32_t nbyte = sizeof(int16_t);
    const int32_t ndim = 2;  // always 2 dimentions (channels and time)

    fwrite(&type, sizeof(type), 1, fp);
    fwrite(&nbyte, sizeof(nbyte), 1, fp);
    fwrite(&ndim, sizeof(ndim), 1, fp);

    fwrite(&nchan, sizeof(nchan), 1, fp);
    fwrite(&npt, sizeof(npt), 1, fp);
}
/* -------------------------------------------------------------------------- */
int get_channel_type(const char* smrfile, int idx)
{
    struct SMRChannelInfoArray* ifo = read_channel_array(smrfile);

    int type = -1;

    if (ifo != NULL)
    {
        for (size_t k = 0; k < ifo->length; ++k)
        {
            if (ifo->ifo[k]->index == idx)
            {
                type = (int)ifo->ifo[k]->kind;
                break;
            }
        }
        free_channel_info_array(ifo);
    }
    return type;
}
/* -------------------------------------------------------------------------- */
double get_sampling_rate(const char* smrfile, int idx)
{
    double fs = 0.0;

    struct SMRFileHeader* fhdr = read_file_header(smrfile);

    if (fhdr != NULL)
    {
        fs = MICROSECONDS / get_sample_interval(fhdr, idx);
        free_file_header(fhdr);
    }
    else
    {
        printf("[WARN]: Failed to find sampling rate for channel %d", idx);
    }
    return fs;
}
/* -------------------------------------------------------------------------- */
int write_wavemark(FILE* fp, const char* smrfile, int idx,
    int write_header, size_t nchan, uint32_t* npt, double* fs)
{
    int success = 0;
    struct SMRWMrkChannel* wmrk = read_wavemark_channel(smrfile, idx);

    if (wmrk != NULL)
    {
        // we double the number of wavemarks due to the npt zero padding between
        // each wavemark

        if (write_header)
        {
            *npt = (wmrk->length * 2 * wmrk->npt);
            write_mda_header(fp, nchan, *npt);
        }

        if ((*npt) ==  (wmrk->length * 2 * wmrk->npt))
        {
            const int16_t z = 0;

            for (size_t k = 0; k < wmrk->length; ++k)
            {
                fwrite(wmrk->wavemarks + (wmrk->npt * k), sizeof(int16_t), wmrk->npt, fp);
                for (size_t j = 0; j < wmrk->npt; ++j)
                {
                    fwrite(&z, sizeof(int16_t), 1, fp);
                }
            }
            success = 1;
        }
        else
        {
            show_error("Not all channels have the same # of samples!");
        }

        *fs = get_sampling_rate(smrfile, idx);
        free_wavemark_channel(wmrk);
    }

    return success;
}
/* -------------------------------------------------------------------------- */
int write_continuous(FILE* fp, const char* smrfile, int idx,
    int write_header, size_t nchan, uint32_t* npt, double* fs)
{
    int success = 0;
    struct SMRContChannel* cont = read_continuous_channel(smrfile, idx);

    if (cont != NULL)
    {
        if (write_header)
        {
            *npt = cont->length;
            write_mda_header(fp, nchan, *npt);
        }

        if ((*npt) == cont->length)
        {
            fwrite(cont->data, sizeof(int16_t), cont->length, fp);
            success = 1;
        }
        else
        {
            show_error("Not all channels have the same # of samples!");
        }

        *fs = cont->sampling_rate;
        free_continuous_channel(cont);
    }

    return success;
}
/* -------------------------------------------------------------------------- */
int write_mda(const char* smrfile, const char* mdafile, IntArray* channels)
{
    int exit_code = 0;

    if (channels->length < 1)
    {
        printf(
            "[INFO]: no channels given, aborting... "
            "(mda file would be empty)\n"
        );
        return exit_code;
    }

    FILE *mda_fp = open_file(mdafile, FILE_WRITE_MODE);

    if (mda_fp != NULL)
    {
        uint32_t npt = 0;
        double sampling_rate;
        int success;
        for (int32_t k = 0; k < channels->length; ++k)
        {
            int write_header = k == 0 ? 1 : 0;

            int type = get_channel_type(smrfile, channels->data[k]);

            switch (type)
            {
                case CONTINUOUS_CHANNEL:
                    success = write_continuous(mda_fp, smrfile, channels->data[k],
                        write_header, channels->length, &npt, &sampling_rate);
                    break;

                case ADC_MARKER_CHANNEL:
                    if (channels->length > 1)
                    {
                        show_error("Multiple wavemark channels cannot by saved to a single MDA file");
                        exit_code = -5;
                    }
                    else
                    {
                        success = write_wavemark(mda_fp, smrfile,
                            channels->data[k], write_header, channels->length,
                            &npt, &sampling_rate);
                    }
                    break;

                default:
                    show_error("Only continuous and wavemark channels are supported");
                    printf("    Channel Type: %d\n", type);
                    exit_code = -4;
                    break;
            }

            if (!success)
            {
                show_error("Failed to write channel to MDA file");
                printf("    Channel Index: %d\n", channels->data[k]);
                exit_code = -3;
            }
        }

        fclose(mda_fp);

        if (exit_code != 0)
        {
            remove(mdafile);
        }
        else
        {
            char *paramfile = swap_ext(mdafile, "json");
            FILE *param_fp = open_file(paramfile, FILE_WRITE_MODE);

            if (param_fp != NULL)
            {
                char buf[64];
                sprintf(buf, "{\"samplerate\": %d}",
                    (int32_t)round(sampling_rate));

                fwrite(buf, sizeof(char), strlen(buf), param_fp);
                fclose(param_fp);
            }
            else
            {
                show_error("Failed to open json file for writing params!");
            }

            free(paramfile);
        }
    }
    else
    {
        show_error("Failed to open file for writing!");
        printf("    Invalid file: %s\n", mdafile);
        exit_code = -2;
    }

    return exit_code;
}
/* -------------------------------------------------------------------------- */
void usage()
{
    printf(
        "\nUsage:\n"
        "smr2mda [options] <smrfile> <mdafile>\n"
        "\n"
        "Options:\n"
        "    l       - print channel info for <smrfile> (output optional)\n"
        "    c [idx] - only include channels from <smrfile> with indicies\n"
        "              [idx] in output. [idx] should be a comma seperated\n"
        "              list of integer channel indicies (e.g. \"1,2\")\n"
        "    h       - print documentation\n"
        "\n"
        "Inputs:\n"
        "    <smrfile> - full path to a Spike2 SMR file\n"
        "    <mdafile> - full path to write the resulting mda file to\n"
        "\n"
        "Examples:\n"
        "    #for printing channel info, output path is not needed\n"
        "    smr2mda -l ./b1_con_006.smr\n"
        "\n"
        "    #convert all continuous channels\n"
        "    smr2mda ./b1_con_006.smr ./test.mda\n"
        "\n"
        "    #only convert channels 12 and 13 (NOTE: these are the channel INDICIES)\n"
        "    smr2mda -c \"12,13\" ./b1_con_006.smr ./test2.mda\n"
        "\n"
    );
}
/* -------------------------------------------------------------------------- */
int main(int argc, char const *argv[]) {

    int exit_code = 0;

    if (argc < 2)
    {
        show_error("Not enough input arguments!");
        return 127;
    }

    if (argv[1][0] == '-')
    {
        switch (argv[1][1]) {
            case 'l':
                if (argc > 2)
                {
                    dump_channel_info(argv[2]);
                }
                break;
            case 'c':
                if (argc < 5)
                {
                    show_error("Not enough input arguments!");
                    exit_code = 126;
                }
                else
                {
                    IntArray *channels = get_indicies_from_list(argv[2]);
                    if (channels != NULL)
                    {
                        exit_code = write_mda(argv[3], argv[4], channels);
                    }
                    free_int_array(channels);
                }
                break;
            case 'h':
                usage();
                break;
            default:
                show_error("Unrecognized flag!");
        }
    }
    else if (argc > 2)
    {
        IntArray *channels = get_continuous_indicies(argv[1]);
        if (channels != NULL)
        {
            exit_code = write_mda(argv[1], argv[2], channels);
        }
        free_int_array(channels);
    }
    else
    {
        show_error("Not enough input arguments for call syntax!");
        return 127;
    }

    return exit_code;
}
/* -------------------------------------------------------------------------- */
