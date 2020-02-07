#include <string.h>
#include "matrix.h"
#include "mex.h"
#include "smr.h"

/* ========================================================================= */
mxArray *get_continuous_channel(const char *ifile, int idx)\
{
    mxArray *out;

    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;
    struct SMRContChannel *chan = NULL;

    double *data_ptr, *fs_ptr;
    double scale, offset;
    int fail = 1;
    long unsigned int k;

    const char *fields[] = {"data", "sampling_rate"};
    out = mxCreateStructMatrix(1, 1, 2, fields);

    if ((fhdr = read_file_header(ifile)) == NULL)
    {
        goto cleanup;
    }

    if ((chdr = read_channel_header(fhdr, idx)) == NULL)
    {
        goto cleanup;
    }

    scale = (double) chdr->scale / 6553.6;
    offset = (double) chdr->offset;

    if ((chan = read_continuous_channel_from_header(fhdr, chdr)) != NULL)
    {
        mxArray *data = mxCreateNumericMatrix(chan->length, 1, mxDOUBLE_CLASS, mxREAL);
        mxArray *fs = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);

        data_ptr = mxGetData(data);

        /* convert continuous data saved as int16 to voltage as
           volts = (data * (scale / 6553.6)) + offset */
        for (k = 0; k < chan->length; ++k)
        {
            *(data_ptr++) = (chan->data[k] * scale) + offset;
        }

        fs_ptr = mxGetPr(fs);
        *fs_ptr = chan->sampling_rate;

        mxSetField(out, 0, "data", data);
        mxSetField(out, 0, "sampling_rate", fs);

        free_continuous_channel(chan);

        fail = 0;
    }

cleanup:
    free_channel_header(chdr);
    free_file_header(fhdr);

    if (fail)
    {
        mexPrintf("WARNING: failed to read channel [%d] from file %s\n", idx, ifile);
    }

    return out;
}
/* ========================================================================= */
mxArray *get_wavemark_channel(const char *ifile, int idx)
{

    struct SMRWMrkChannel *chan = NULL;
    mxArray *out;

    const char *fields[] = {"timestamps", "markers", "wavemarks"};
    out = mxCreateStructMatrix(1, 1, 3, fields);

    if ((chan = read_wavemark_channel(ifile, idx)) != NULL) {

        mxArray *ts = mxCreateNumericMatrix(chan->length, 1, mxDOUBLE_CLASS, mxREAL);
        mxArray *mrk = mxCreateNumericMatrix(MARKER_SIZE, chan->length, mxUINT8_CLASS, mxREAL);
        mxArray *wmrk = mxCreateNumericMatrix(chan->npt, chan->length, mxINT16_CLASS, mxREAL);

        memcpy(mxGetPr(ts), chan->timestamps, (size_t) chan->length * sizeof (double));
        memcpy(mxGetData(mrk), chan->markers, (size_t) chan->length * MARKER_SIZE * sizeof (uint8_t));
        memcpy(mxGetData(wmrk), chan->wavemarks, (size_t) chan->length * chan->npt * sizeof (int16_t));

        mxSetField(out, 0, "timestamps", ts);
        mxSetField(out, 0, "markers", mrk);
        mxSetField(out, 0, "wavemarks", wmrk);

        free_wavemark_channel(chan);

    }
    else
    {
        mexPrintf("WARNING: failed to read channel [%d] from file %s\n", idx, ifile);
    }

    return out;
}
/* ========================================================================= */
mxArray *get_event_channel(const char *ifile, int idx) {

    struct SMREventChannel *chan = NULL;
    mxArray *out;

    if ((chan = read_event_channel(ifile, idx)) != NULL)
    {
        out = mxCreateNumericMatrix(chan->length, 1, mxDOUBLE_CLASS, mxREAL);

        memcpy(mxGetData(out), chan->data, (size_t) chan->length * sizeof (double));

        free_event_channel(chan);
    }
    else
    {
        out = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
        mexPrintf("WARNING: failed to read channel [%d] from file %s\n", idx, ifile);
    }

    return out;
}
/* ========================================================================= */
mxArray *get_marker_channel(const char *ifile, int idx)
{
    struct SMRMarkerChannel *chan = NULL;
    mxArray *out, *ts, *mrk, *txt;

    const char *fields[] = {"timestamps", "markers", "text"};
    out = mxCreateStructMatrix(1, 1, 3, fields);

    if ((chan = read_marker_channel(ifile, idx)) != NULL)
    {
        ts = mxCreateNumericMatrix(chan->length, 1, mxDOUBLE_CLASS, mxREAL);
        mrk = mxCreateNumericMatrix(MARKER_SIZE, chan->length, mxUINT8_CLASS, mxREAL);
        txt = mxCreateNumericMatrix(chan->npt, chan->length, mxUINT8_CLASS, mxREAL);

        memcpy(mxGetPr(ts), chan->timestamps, (size_t) chan->length * sizeof (double));
        memcpy(mxGetData(mrk), chan->markers, (size_t) chan->length * MARKER_SIZE * sizeof (uint8_t));
        memcpy(mxGetData(txt), chan->text, (size_t) chan->length * chan->npt * sizeof (uint8_t));

        mxSetField(out, 0, "timestamps", ts);
        mxSetField(out, 0, "markers", mrk);
        mxSetField(out, 0, "text", txt);

        free_marker_channel(chan);

    }
    else
    {
        mexPrintf("WARNING: failed to read channel [%d] from file %s\n", idx, ifile);
    }

    return out;
}
/* ========================================================================= */
void mexFunction(int nout, mxArray *pout[], int nin, const mxArray *pin[])
{
    char *ifile, *label;
    struct SMRFileHeader *fhdr = NULL;
    struct SMRChannelHeader *chdr = NULL;

    int idx;

    if (nin < 2)
    {
        mexErrMsgTxt("Not enough inputs!");
    }

    if (!mxIsChar(pin[0]))
    {
        mexErrMsgTxt("Input 1 *MUST* be a file path [string]");
    }

    ifile = mxArrayToString(pin[0]);

    if ((fhdr = read_file_header(ifile)) != NULL)
    {
        if (mxIsChar(pin[1]))
        {
            label = mxArrayToString(pin[1]);
            idx = channel_label_to_index(fhdr, label);

            mxFree(label);

            if (idx < 0)
            {
                free_file_header(fhdr);
                mxFree(ifile);
                mexErrMsgTxt("Input 2 *MUST* be a valid channel label [string] or index [number]");
            }

        }
        else if (mxIsNumeric(pin[1]))
        {
            idx = (int) mxGetScalar(pin[1]);
        }

        if ((chdr = read_channel_header(fhdr, idx)) != NULL) {
            switch (chdr->kind) {
                case CONTINUOUS_CHANNEL:
                    pout[0] = get_continuous_channel(ifile, idx);
                    break;

                case EVENT_2_CHANNEL:
                case EVENT_3_CHANNEL:
                case EVENT_4_CHANNEL:
                    pout[0] = get_event_channel(ifile, idx);
                    break;

                case MARKER_CHANNEL:
                case TEXT_MARKER_CHANNEL:
                    pout[0] = get_marker_channel(ifile, idx);
                    break;

                case ADC_MARKER_CHANNEL:
                    pout[0] = get_wavemark_channel(ifile, idx);
                    break;

                default:
                    pout[0] = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                    mexPrintf("WARNING: channels of type %d are not yet supported\n", chdr->kind);
            }
            free_channel_header(chdr);
        }
        else
        {
            fprintf(stderr, "ERROR: Failed to read channel header\n");
        }
        free_file_header(fhdr);
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to read file header\n");
    }
    mxFree(ifile);
}
/* ========================================================================= */
