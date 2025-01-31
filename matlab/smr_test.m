function smr_test(ifile)
% smr_test
%
% Description: run a test of the libsmr matlab interface by reading channel
%              info and then every channel based on that info from a test file
%
% Syntax: smr_test(ifile)
%
% In:
%       ifile - the full path to a .smr file
%
% Out:
%
% Updated: 2016-04-05
% Scottie Alexander
%
% Please report bugs to: scottiealexander11@gmail.com

if exist(ifile, 'file') ~= 2
    error('Failed to locate smr test file: looking for "%s"', ifile);
end

if ispc()
    mex_dir = 'windows';
else
    if ismac()
        mex_dir = 'darwin';
    else
        mex_dir = 'linux';
    end
end

cdir = pwd;
cd(fullfile(pwd, mex_dir));

try
    ifo = smr_channel_info(ifile);

    for k = 1:numel(ifo)
        fprintf('reading channel: [%d] %s... ', ifo(k).index, ifo(k).label);

        % reading channels by their index also works, see commented line below
        % tmp = smr_read_channel(ifile, ifo(k).index);
        tmp = smr_read_channel(ifile, ifo(k).label);

        fprintf('success!\n');
    end
catch me
    cd(cdir);
    rethrow(me);
end
cd(cdir);

end
