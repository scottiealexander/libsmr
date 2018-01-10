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
int write_mda(const char *smrfile, const char *mdafile, IntArray *channels)
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
        uint32_t npt;
        double sampling_rate;
        for (int32_t k = 0; k < channels->length; ++k)
        {
            struct SMRContChannel *cont = read_continuous_channel(smrfile,
                channels->data[k]);

            if (cont != NULL)
            {
                if (k == 0)
                {
                    sampling_rate = cont->sampling_rate;
                    npt = (uint32_t)cont->length;
                    write_mda_header(mda_fp, channels->length, npt);
                }
                else if ((uint32_t)cont->length != npt)
                {
                    show_error("Not all channels have the same # of samples!");
                    free_continuous_channel(cont);
                    exit_code = -4;
                    break;
                }

                fwrite(cont->data, sizeof(int16_t), cont->length, mda_fp);
                free_continuous_channel(cont);
            }
            else
            {
                exit_code = -3;
                break;
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
