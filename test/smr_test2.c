
#include <stdio.h>
#include "smr.h"
#include "./_dev/smr_display.h"

uint8_t test_all()
{
    const char *ifile = "/media/data/sorting/3373/w3373a_ch1.smr";
    // const char *ifile = "/media/scottie/KINGSTON/libsmr/b1_con_006.smr";

    const char *wmrk_label = "nw-2";
    const char *cont_label = "Raw";

    int idx;

    printf("read_file_header: ");
    struct SMRFileHeader *fhdr = read_file_header(ifile);

    if (fhdr)
    {
        printf("success\n");
    }
    else
    {
        printf("failure\n");
        return 0;
    }

    printf("read_channel_info_array: ");
    struct SMRChannelInfoArray *ifo = read_channel_info_array(fhdr);

    if (ifo)
    {
        printf("success\n");
    }
    else
    {
        printf("failure\n");
        free_file_header(fhdr);
        return 0;
    }

    free_channel_info_array(ifo);

    idx = channel_label_to_index(fhdr, cont_label);

    printf("read_channel_header: ");
    struct SMRChannelHeader *chdr = read_channel_header(fhdr, idx);

    if (chdr)
    {
        printf("success\n");
        // print_channel_header(chdr);
        // free_channel_header(chdr);
        // free_file_header(fhdr);
        // return 1;
    }
    else
    {
        printf("failure\n");
        free_file_header(fhdr);
        return 0;
    }

    unsigned long int si = get_sample_interval(fhdr, idx);
    printf("\tsample interval of channel %d = %lu\n", idx, si);


    struct SMRContChannel *cont = read_continuous_channel(ifile, idx);

    if (cont)
    {
        printf("read_continuous_channel: success\n");
        printf("\tchannel length: %02lu\n", cont->length);
        free_continuous_channel(cont);
    }
    else
    {
        free_channel_header(chdr);
        free_file_header(fhdr);
        return 0;
    }

    free_channel_header(chdr);

    free_file_header(fhdr);

    return 1;
}
/* ========================================================================= */

int main()
{
    if (test_all())
    {
        printf("***ALL TESTES PASS***\n");
    }
    else
    {
        printf("FAILURE DETECTED\n");
    }
    return 0;
}
/* ========================================================================= */
/*
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./smr_test
*/
