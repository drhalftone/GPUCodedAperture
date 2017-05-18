function y = mydct(x)

y = zeros(size(x));
for m=1:size(x,1)
    for n=1:size(x,2)
        w = x(m,n,:);
        w = dct(w(:));
        y(m,n,:) = w(:);
    end;
end;

return; 