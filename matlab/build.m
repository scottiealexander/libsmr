function build(varargin)
% build
%
% Description: build script for matlab interface to libsmr
%
% Syntax: build([install_dir])
%
% In:
%       install_dir - the directory to install mex files to
%
% Out:
%
% Updated: 2016-05-13
% Scottie Alexander
%
% Please report bugs to: scottiealexander11@gmail.com

mdir = fileparts(mfilename('fullpath'));
idir = fileparts(mdir);

files = {'smr_read_channel', 'smr_channel_info', 'smr_channel_fs'};

lib_file = fullfile(idir, 'smr.c');

if ispc()
    odir = 'windows';
else
    if ismac()
        odir = 'darwin';
    else
        odir = 'linux';
    end
end

if ~isempty(varargin) && ischar(varargin{1})
    install_dir = varargin{1};
else
    install_dir = '';
end

for k = 1:numel(files)
    ifile = fullfile(mdir, [files{k} '.c']);
    ofile = fullfile(mdir, odir, files{k});

    help_file = [ofile '.m'];

    cmd = {
        '-largeArrayDims',...
        '-output', ofile ,...
        ['-I' idir]      ,...
        ifile            ,...
        lib_file          ...
    };

    mex(cmd{:});

    if isdir(install_dir)
        install_file = fullfile(install_dir, [files{k} '.' mexext()]);
        install_help = fullfile(install_dir, [files{k} '.m']);
        if ~copyfile(ofile, install_file)
            warning('build:CopyFail', 'Failed to copy mex file to %s', install_dir);
        end
        if ~copyfile(help_file, install_help)
            warning('build:CopyFail', 'Failed to copy help file to %s', install_dir);
        end
    elseif ~isempty(install_dir)
        warning('build:InstallFailure', 'Install directory does not exist!');
    else
        %nothing to do, no install_dir was given
    end
end
