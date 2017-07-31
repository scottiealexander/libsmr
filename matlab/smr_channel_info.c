#include "mex.h"
#include "smr.h"

/* ========================================================================== */
void exit_with_error(char *ifile, struct SMRFileHeader *fhdr, char *msg)
{
    mxFree(ifile);
    free_file_header(fhdr);
    mexErrMsgTxt(msg);
}
/* ========================================================================== */
void get_channel_info(struct SMRChannelInfo *ifo, mxArray *out, unsigned int idx)
{
    mxArray *tmp;

    mxSetField(out, idx, "label", mxCreateString(ifo->title));

    tmp = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    *(int32_t *)mxGetData(tmp) = ifo->index;
    mxSetField(out, idx, "index", tmp);

    tmp = mxCreateNumericMatrix(1, 1, mxUINT8_CLASS, mxREAL);
    *(uint8_t *)mxGetData(tmp) = ifo->kind;
    mxSetField(out, idx, "type", tmp);

    tmp = mxCreateNumericMatrix(1, 1, mxINT16_CLASS, mxREAL);
    *(int16_t *)mxGetData(tmp) = ifo->phy_chan;
    mxSetField(out, idx, "port", tmp);
}
/* ========================================================================== */
void mexFunction(int nout, mxArray *pout[], int nin, const mxArray *pin[])
{
    unsigned int k;
    char *ifile;
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelInfoArray *cifo = NULL;

    const char *fields[] = {"label", "index", "type", "port"};

    if (nin < 1)
    {
        mexErrMsgTxt("Not enough inputs!");
    }

    if (!mxIsChar(pin[0]))
    {
        mexErrMsgTxt("Input *MUST* be a file path [string]");
    }

    ifile = mxArrayToString(pin[0]);

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        exit_with_error(ifile, fhdr, "Failed to read file header");
    }

    if ((cifo = read_channel_info_array(fhdr)) == NULL)
    {
        exit_with_error(ifile, fhdr, "Failed to read channel info array");
    }

    pout[0] = mxCreateStructMatrix(cifo->length, 1, 4, fields);

    for (k = 0; k < cifo->length; ++k)
    {
        get_channel_info(cifo->ifo[k], pout[0], k);
    }

    free_channel_info_array(cifo);
    free_file_header(fhdr);

    mxFree(ifile);
}
/* ========================================================================= */

/*
%!gcc -o libsmr.o -g -Wall -c -fPIC smr.c
mex(['-I' pwd], '-output', 'mx_read_channel_info','libsmr.o','mx_read_channel_info.c');
ifile = '/home/scottie/code/c/libsmr/b1_con_006.smr';
ifo = mx_read_channel_info(ifile);
*/
