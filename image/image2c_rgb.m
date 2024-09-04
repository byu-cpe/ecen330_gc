% Clear command window & workspace, and close all figures
clc, clear, close all;

o_max_w = 320; % output image maximum width
o_max_h = 240; % output image maximum height
o_bits = 16; % output image bits per pixel
o_dir = "rgb565"; % output sub-directory

% Select image files to convert
[fname,location] = uigetfile(...
    '*.bmp;*.cur;*.gif;*.hdf4;*.ico;*.jpg;*.jpeg;*.pcx;*.pbm;*.pgm;*.png;*.ppm;*.ras;*.tif;*.tiff;*.xwd',...
    'Select one or more image files',...
    'MultiSelect','on');
if isequal(fname,0) % user canceled selection
    disp('No file(s) selected');
    return;
elseif ischar(fname) % convert to cell array if single file selected
    fname = {fname};
end

% Create output sub-directory if nonexistent
if not(isfolder(o_dir))
    mkdir(o_dir);
end

% Process image data
for i = 1:length(fname)
    % read image file into a matrix
    % returns: [image data, colormap values]
    [x,cmap] = imread(fullfile(location,fname{i}));

    % if indexed (colormapped) image, convert to 24-bit RGB
    if numel(cmap) > 0
        fprintf('Converting: %s to 24-bit RGB.\n', fname{i});
        x = uint8(ind2rgb(x,cmap) .* 255);
    end

    % skip if not in 24-bit RGB format
    if size(x,3) ~= 3 || ~isa(x,'uint8')
        fprintf(' -- error: %s not in 24-bit RGB format.\n', fname{i});
        continue
    end

    % resize image if a dimension is greater than maximum width or height
    if size(x,2) > o_max_w || size(x,1) > o_max_h
        fprintf('Resizing: %s\n', fname{i});
        if size(x,2)/o_max_w > size(x,1)/o_max_h
            xs = imresize(x,[NaN,o_max_w]);
        else
            xs = imresize(x,[o_max_h,NaN]);
        end
    else
        xs = x;
    end

    % show the resized image
    figure, imshow(xs);

    % convert to rgb565
    xr =          bitshift(uint16(bitand(xs(:,:,1),0xF8)), 8); % left by 8
    xr = bitor(xr,bitshift(uint16(bitand(xs(:,:,2),0xFC)), 3)); % left by 3
    xr = bitor(xr,bitshift(uint16(bitand(xs(:,:,3),0xF8)),-3)); % right by 3

    % flatten matrix (row-wise) to a vector
    xr = reshape(xr.',[],1);

    % save data to file in a 'C' array
    [path,name,ext] = fileparts(fname{i}); % split filename
    path = fullfile(path,o_dir); % output to sub-directory
    dat2c(xr,path,name,size(xs,2),size(xs,1),o_bits);
end

% Given a MATLAB array of integer data, create a 'C' array in text.
%   x: MATLAB array of integer data
%   path: directory path to create 'C' file
%   name: name of 'C' array and also files with .h and .c extension
%   w: output image width
%   h: output image height
%   bits: output image bits per pixel
%   Returns the length of the MATLAB array
function l = dat2c(x,path,name,w,h,bits)
    str = upper(name);
    if bits > 16; t_type = "uint32_t"; % target array element type
    elseif bits > 8; t_type = "uint16_t";
    else; t_type = "uint8_t";
    end

    %%%%%%%%%%%%%%%%%%%% Write .h File %%%%%%%%%%%%%%%%%%%%
    fid_h = fopen(fullfile(path,name+".h"), 'w');
    fprintf(fid_h, "\n#include <stdint.h>\n\n");
    fprintf(fid_h, "#define %s_BITS_PER_PIXEL %u\n", str, bits);
    fprintf(fid_h, "#define %s_PIXELS %u\n", str, w*h);
    fprintf(fid_h, "#define %s_W %u\n", str, w);
    fprintf(fid_h, "#define %s_H %u\n\n", str, h);
    fprintf(fid_h, "extern const %s %s[%s_PIXELS];\n", t_type, name, str);
    fclose(fid_h);

    %%%%%%%%%%%%%%%%%%%% Write .c File %%%%%%%%%%%%%%%%%%%%
    ELEM_LINE = 16; % 'C' array elements per line
    t_format = sprintf(" 0x%%0%ux,", bits/4);
    fid_c = fopen(fullfile(path,name+".c"), 'w');
    pos = 0;
    elem = length(x);

    fprintf(fid_c, "\n#include <stdint.h>\n\n");
    fprintf(fid_c, "const %s %s[] = {\n", t_type, name); % start array
    while elem > 0 % array data
        if elem < ELEM_LINE; size = elem; else; size = ELEM_LINE; end
        for i = 1:size
            % if i == 1; fprintf(fid_c, "\t"); end
            fprintf(fid_c, t_format, x(pos+i));
        end
        pos = pos+size;
        elem = elem-size;
        fprintf(fid_c, "\n");
    end
    fprintf(fid_c, "};\n"); % end array
    fclose(fid_c);

    l = length(x);
end
