function fileData(data,filename)
size=length(data(1,:));
fid = fopen(filename,'w');
fprintf(fid,'A0,A1,A2,A3,A4,A5,A6,A7\n');
for j=1:size
    for i=1:8
        if(i~=8)
            fprintf(fid,'%u,',data(i,j));
        else
            fprintf(fid,'%u\n',data(i,j));
        end
    end
end
fclose(fid);