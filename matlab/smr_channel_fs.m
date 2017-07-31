function varargout = smr_channel_fs(ifile, name)
% smr_channel_fs
%
% Syntax: fs = smr_channel_fs(ifile, channel)
%
% In:
%       ifile - the path to a Spike2 .smr file
%       channel - a channel label [string] or channel index [number]
%
% Out:
%       fs - the channel sampling rate in Hz
%
% See also: smr_channel_info
%
% Bugs: Please send bug reports to scottiealexander11@gmail.com
%
% Updated: 2016-05-13
% Scottie Alexander

mex_file = [mfilename('fullpath') '.' mexext];

if exist(mex_file, 'file') ~= 2
    msg = [
    'The required mex file "%s" appears to be missing!\nCheck mex file ', ...
    'install location.'...
    ];
    error(msg, mex_file);
end
