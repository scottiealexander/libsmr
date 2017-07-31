function varargout = smr_read_channel(ifile, channel)
% smr_read_channel
%
% Syntax: data = smr_read_channel(ifile, channel)
%
% In:
%       ifile - the path to a Spike2 .smr file
%       channel - a channel label [string] or channel index [number]
%
% Out:
%       data - a struct with the data from the specified channel
%              different channel type result in different struct formats
%
% See also: smr_channel_info
%
% Bugs: Please send bug reports to scottiealexander11@gmail.com
%
% Updated: 2016-04-01
% Scottie Alexander

mex_file = [mfilename('fullpath') '.' mexext];

if exist(mex_file, 'file') ~= 2
    msg = [
    'The required mex file "%s" appears to be missing!\nCheck mex file ', ...
    'install location.'...
    ];
    error(msg, mex_file);
end
