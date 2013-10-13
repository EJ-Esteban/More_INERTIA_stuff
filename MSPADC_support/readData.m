function a_out=readData(filename)
raw = fread(fopen(filename));

size = length(raw);
if(mod(size,16)~=0)
    disp('Error: obviously incorrect amount of data');
    data=0;
else
    nsamps=size/16;
    data = zeros(8,size/16);
    for j=1:nsamps
        for i=1:8
            k = 16*(j-1)+2*(i-1)+1;
            data(i,j) = 256*raw(k)+raw(k+1);
        end
    end
end
a_out=data;