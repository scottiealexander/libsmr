#include "mex.h"
#include "smr.h"

void mexFunction(int nout, mxArray *pout[], int nin, const mxArray *pin[])
{
    double sample_interval;
    int idx;
    char *ifile, *label;
    struct SMRFileHeader *fhdr;

    if (nin < 1) {
        mexErrMsgTxt("Not enough inputs!");
    }

    if (!mxIsChar(pin[0])) {
        mexErrMsgTxt("Input *MUST* be a file path [string]");
    }

    ifile = mxArrayToString(pin[0]);

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        mxFree(ifile);
        mexErrMsgTxt("Failed to read file header");
    }

    /* convert channel label as a string to channel index (int)*/
    if (mxIsChar(pin[1]))
    {
        label = mxArrayToString(pin[1]);
        idx = channel_label_to_index(fhdr, label);

        mxFree(label);

        if (idx < 0)
        {
            mxFree(ifile);
            free_file_header(fhdr);

            mexErrMsgTxt("Input 2 *MUST* be a valid channel label [string] or index [number]");
        }

    }
    else if (mxIsNumeric(pin[1]))
    {
        idx = (int) mxGetScalar(pin[1]);
    }

    sample_interval = get_sample_interval(fhdr, idx);

    pout[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    *mxGetPr(pout[0]) = MICROSECONDS / sample_interval;

    free_file_header(fhdr);

    mxFree(ifile);
}
