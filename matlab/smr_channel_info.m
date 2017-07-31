function varargout = smr_channel_info(ifile)
% smr_channel_info
%
% Syntax: ifo = smr_channel_info(ifile)
%
% In:
%       ifile - the path to a Spike2 .smr file
%
% Out:
%       ifo - a nchannel x 1 array of structs of information about the channels
%             contained in the smr file, fields:
%               label:    the channel label (non-unique)
%               index:    the Spike2 index of the channel (unique)
%               type:     the type of channel (open this file in a text editor
%                         for more info)
%               port:     the physical channel / 1401 port that the channel
%                         was recorded on
%
% See also: smr_read_channel
%
% Bugs: Please send bug reports to scottiealexander11@gmail.com
%
% Updated: 2016-04-01
% Scottie Alexander

%------------------------------------------------------------------------------%
% channel "port" mapping
%------------------------------------------------------------------------------%
% 1 = CONTINUOUS_CHANNEL,
% 2 = EVENT_RISING_EDGE_CHANNEL,
% 3 = EVENT_FALLING_EDGE_CHANNEL,
% 4 = EVENT_RAISING_FALLING_CHANNEL,
% 5 = MARKER_CHANNEL,
% 6 = ADC_MARKER_CHANNEL,
% 7 = REAL_MARKER_CHANNEL,
% 8 = TEXT_MARKER_CHANNEL,
% 9 = REAL_WAVE_CHANNEL

mex_file = [mfilename('fullpath') '.' mexext];

if exist(mex_file, 'file') ~= 2
    msg = [
    'The required mex file "%s" appears to be missing!\nCheck mex file ', ...
    'install location.'...
    ];
    error(msg, mex_file);
end
