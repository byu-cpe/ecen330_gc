% Clear command window & workspace, and close all figures
clc, clear, close all;

o_max_w = 320; % output image maximum width
o_max_h = 240; % output image maximum height
o_bits = 1; % output image bits per pixel
o_dir = "mono"; % output sub-directory

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
    dat2c_pack(xr,path,name,size(xs,2),size(xs,1),o_bits);
end

% Given a MATLAB array of integer data, create a 'C' array in text.
% Convert image to monochrome bitmap and pack 8 pixels per byte.
% Non-zero pixel values will correspond to bits set to one in the 'C' array.
%   x: MATLAB array of integer data
%   path: directory path to create 'C' file
%   name: name of 'C' array and also files with .h and .c extension
%   w: output image width
%   h: output image height
%   bits: output image bits per pixel
%   Returns the length of the MATLAB array
function l = dat2c_pack(x,path,name,w,h,bits)
    str = upper(name);
    elem = ceil(w/8); % array elements per line
    len = elem*h;
    t_type = "uint8_t";

    %%%%%%%%%%%%%%%%%%%% Write .h File %%%%%%%%%%%%%%%%%%%%
    fid_h = fopen(fullfile(path,name+".h"), 'w');
    fprintf(fid_h, "\n#include <stdint.h>\n\n");
    fprintf(fid_h, "#define %s_BITS_PER_PIXEL %u\n", str, bits);
    fprintf(fid_h, "#define %s_LENGTH %u\n", str, len);
    fprintf(fid_h, "#define %s_W %u\n", str, w);
    fprintf(fid_h, "#define %s_H %u\n\n", str, h);
    fprintf(fid_h, "extern const %s %s[%s_LENGTH];\n", t_type, name, str);
    fclose(fid_h);

    %%%%%%%%%%%%%%%%%%%% Write .c File %%%%%%%%%%%%%%%%%%%%
    fid_c = fopen(fullfile(path,name+".c"), 'w');

    fprintf(fid_c, "\n#include <stdint.h>\n\n");
    fprintf(fid_c, "const %s %s[] = {\n", t_type, name); % start array
    for j = 0:h-1
        for i = 0:elem-1
            tmp = 0;
            for b = 1:8
                if (i*8+b <= w); pval = x(j*w+i*8+b); else; pval = 0; end
                if (pval == 0); tmp = bitshift(tmp,1);
                else; tmp = bitor(1,bitshift(tmp,1)); end
            end
            fprintf(fid_c, " 0x%02x,", tmp);
        end
        fprintf(fid_c, "\n");
    end
    fprintf(fid_c, "};\n"); % end array
    fclose(fid_c);

    l = length(x);
end
