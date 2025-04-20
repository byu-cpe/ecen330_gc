% Clear command window & workspace, and close all figures
clc, clear, close all;

t_fs = 32000;       % target sample frequency
t_bits = 16;        % target bits per sample
t_dir = "c32k_16b"; % target sub-directory
% Comment out t_amp to leave the signal amplitude (volume) the same
% t_amp = 1.0;        % target max amplitude [0.0 to 1.0]

% Select audio files to convert
[fname,location] = uigetfile(...
    '*.aifc;*.aiff;*.aif;*.au;*.flac;*.ogg;*.opus;*.mp3;*.m4a;*.mp4;*.wav',...
    'Select one or more audio files',...
    'MultiSelect','on');
if isequal(fname,0) % user canceled selection
    disp('No file(s) selected');
    return;
elseif ischar(fname) % convert to cell array if single file selected
    fname = {fname};
end

% Create output sub-directory if nonexistent
if not(isfolder(t_dir))
    mkdir(t_dir);
end

% Process audio data
for i = 1:length(fname)
    % read audio wave file into a matrix
    % returns: [data, sample frequency]
    [x, fs] = audioread(fullfile(location,fname{i}));

    % combine any channels (e.g. stereo to mono)
    x1 = mean(x,2);

    % resample at target sample frequency
    [P,Q] = rat(t_fs/fs);
    xs = resample(x1,P,Q);

    % play original and resampled audio back-to-back
    % playblocking(audioplayer(x1,fs));
    % playblocking(audioplayer(xs,t_fs));

    % rescale data to the interval [-2^(t_bits-1)+1, 2^(t_bits-1)-1]
    gain = (2 .^ (t_bits-1))-1;
    if exist('t_amp','var') == 1 % if t_amp exists, set the max amplitude
        % change the volume if t_amp is [0.0 to 1.0]
        max_amp = max(abs(min(xs)),abs(max(xs)));
        gain = gain*t_amp/max_amp;
    end
    xr = int32(xs .* gain);
    xr = min(xr,( 2 .^ (t_bits-1))-1); % clip to ( 2^(t_bits-1))-1
    xr = max(xr,(-2 .^ (t_bits-1))+1); % clip to (-2^(t_bits-1))+1

    % figure;
    % plot(xr);

    % save data to file in a 'C' array
    [path,name,ext] = fileparts(fname{i}); % split filename
    path = fullfile(path,t_dir); % output to sub-directory
    dat2c(xr,path,name,t_fs,t_bits);
end

% Given a MATLAB array of signed integer data, create a 'C' array in text
%   xi: MATLAB array of signed integer data
%   path: directory path to create 'C' file
%   name: name of 'C' array and also files with .h and .c extension
%   fs: sample frequency of data
%   bits: bits per sample of data
%   Returns the length of the MATLAB array
function l = dat2c(xi,path,name,fs,bits)
    if bits > 16
        e_type = "int32_t"; % array element type
        e_format = " %11d,"; % array element format string
        e_line = 8; % 'C' array elements per line
    elseif bits > 8
        e_type = "int16_t";
        e_format = " %6d,";
        e_line = 16;
    else
        e_type = "int8_t";
        e_format = " %4d,";
        e_line = 16;
    end
    str = upper(name);

    %%%%%%%%%%%%%%%%%%%% Write .h File %%%%%%%%%%%%%%%%%%%%
    fid_h = fopen(fullfile(path,name+".h"), 'w');
    fprintf(fid_h, "\n#include <stdint.h>\n\n");
    fprintf(fid_h, "#define %s_BITS_PER_SAMPLE %u\n", str, bits);
    fprintf(fid_h, "#define %s_SAMPLE_RATE %u\n", str, fs);
    fprintf(fid_h, "#define %s_SAMPLES %u\n\n", str, length(xi));
    fprintf(fid_h, "extern const %s %s[%s_SAMPLES];\n", e_type, name, str);
    fclose(fid_h);

    %%%%%%%%%%%%%%%%%%%% Write .c File %%%%%%%%%%%%%%%%%%%%
    fid_c = fopen(fullfile(path,name+".c"), 'w');
    pos = 0;
    elem = length(xi);

    fprintf(fid_c, "\n#include <stdint.h>\n\n");
    fprintf(fid_c, "const %s %s[] = {\n", e_type, name); % start array
    while elem > 0 % array data
        if elem < e_line; size = elem; else; size = e_line; end
        for i = 1:size
            % if i == 1; fprintf(fid_c, "\t"); end
            fprintf(fid_c, e_format, xi(pos+i));
        end
        pos = pos+size;
        elem = elem-size;
        fprintf(fid_c, "\n");
    end
    fprintf(fid_c, "};\n"); % end array
    fclose(fid_c);

    l = length(xi);
end
