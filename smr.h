#ifndef _SMR_H
#define _SMR_H

#include <stdint.h>
#include <stdio.h>

/* =============================================================================
TODO:
    1) rename field 'nitem' to 'nsample' in block header
    2) look into sizeof size_t, is it always as big as long long int?
    5) in theory all marker reads could be done with one function
       fields: timestamps, markers, data (wavemarks / text)
    6) scale data to physicaly meaningful values
============================================================================= */
/*number of elements in a single marker data point*/
#define MARKER_SIZE 4

/*block header is 20 bytes*/
#define BLOCK_HEADER_SIZE 20

/*# of microsconds in a second*/
#define MICROSECONDS 1000000.0

/* ========================================================================== */
enum channel_type_t
{
    CONTINUOUS_CHANNEL = 1,
    EVENT_2_CHANNEL = 2,
    EVENT_3_CHANNEL = 3,
    EVENT_4_CHANNEL = 4,
    MARKER_CHANNEL = 5,
    ADC_MARKER_CHANNEL = 6,
    REAL_MARKER_CHANNEL = 7,
    TEXT_MARKER_CHANNEL = 8,
    REAL_WAVE_CHANNEL = 9
};
/* ========================================================================== */
struct SMRFileHeader
{
    char *filepath;

    int16_t system_id;

    char copyright[11];
    char creator[9];

    int16_t uspertime;
    int16_t timeperadc;
    int16_t filestate;

    int32_t firstdata;

    int16_t nchannel;
    int16_t chansize;
    int16_t extra_data;
    int16_t buffersize;
    int16_t osformat;

    int32_t maxtime;

    double dtimebase;

    uint8_t time_detail[6];
    int16_t time_year;

    char pad[53];

    char *comment[5];
};
/* ========================================================================== */
struct SMRChannelHeader
{
    char *filepath;
    int index;

    int16_t del_size;
    int32_t next_del_block;
    int32_t first_block;
    int32_t last_block;

    int16_t nblock;
    int16_t nextra;
    int16_t pre_trig;
    int16_t free_0;
    int16_t phy_sz;
    int16_t max_data;

    char *comment;

    int32_t max_chan_time;
    int32_t l_chan_dvd;
    int16_t phy_chan;

    char *title;

    float ideal_rate;
    uint8_t kind;
    int8_t pad;

    float scale;
    float offset;

    char *units;

    int16_t divide;
    int16_t interleave;

    float min;
    float max;

    unsigned char init_low;
    unsigned char next_low;
};
/* -------------------------------------------------------------------------- */
struct SMRChannelInfo
{
    char *title;
    int index;
    uint8_t kind;
    int16_t phy_chan;
};
/* -------------------------------------------------------------------------- */
struct SMRChannelInfoArray
{
    unsigned int length;
    struct SMRChannelInfo **ifo;
};
/* ========================================================================== */
struct SMRBlockHeader
{
    int32_t next_block;
    int32_t last_block;
    int32_t start_time;
    int32_t end_time;

    int16_t index;
    int16_t nitem;
};
/* -------------------------------------------------------------------------- */
struct SMRBlockHeaderArray
{
    unsigned int length;
    struct SMRBlockHeader *hdr;
};
/* ========================================================================== */
struct SMRWMrkChannel
{
    uint64_t length; /*# of spikes*/
    uint64_t npt;    /*points-per-spike*/
    double *timestamps;
    uint8_t *markers;
    int16_t *wavemarks;
};
/* ========================================================================== */
struct SMRContChannel
{
    uint64_t length;
    double sampling_rate;
    int16_t *data;
};
/* ========================================================================== */
struct SMREventChannel
{
    uint64_t length;
    double *data;
};
/* ========================================================================== */
struct SMRMarkerChannel
{
    uint64_t length; /*number of events*/
    uint64_t npt;    /*# of chars for each event*/
    double *timestamps;
    uint8_t *markers;
    uint8_t *text;
};
/* ========================================================================== */
struct SMRFileHeader *read_file_header(const char *);
void free_file_header(struct SMRFileHeader *);

struct SMRChannelHeader *read_channel_header(struct SMRFileHeader *, int);
void free_channel_header(struct SMRChannelHeader *);

struct SMRBlockHeaderArray *read_block_header_array(struct SMRChannelHeader *);
struct SMRBlockHeader read_block_header(FILE *);
void free_block_header_array(struct SMRBlockHeaderArray *);

struct SMRChannelInfoArray *read_channel_info_array(struct SMRFileHeader *);
struct SMRChannelInfo *load_channel_info(struct SMRChannelHeader *);
void free_channel_info_array(struct SMRChannelInfoArray *);
void free_channel_info(struct SMRChannelInfo *);

struct SMRWMrkChannel *read_wavemark_channel(const char *, int);
void free_wavemark_channel(struct SMRWMrkChannel *);

struct SMRContChannel *read_continuous_channel(const char *, int);
struct SMRContChannel *read_continuous_channel_from_header(
    struct SMRFileHeader *, struct SMRChannelHeader *);
void free_continuous_channel(struct SMRContChannel *);

struct SMREventChannel *read_event_channel(const char *, int);
void free_event_channel(struct SMREventChannel *);

struct SMRMarkerChannel *read_marker_channel(const char *, int);
void free_marker_channel(struct SMRMarkerChannel *);

int channel_label_to_index(struct SMRFileHeader *, const char *);

int channel_label_path_to_index(const char *, const char *);

double get_sample_interval(struct SMRFileHeader *, int);

struct SMRChannelInfoArray *read_channel_array(const char *);

/* ========================================================================== */
#endif
