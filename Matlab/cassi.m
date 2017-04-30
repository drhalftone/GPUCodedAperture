function y = cassi(x,ca)

if (nargin > 1)
    x = x.*ca;
end;

y = zeros(size(x,1),size(x,2)+7,8);
for l=1:8
    y(:,9-l:8-l+size(x,2),l) = x;
end;
x = y/8;

w = spatial(x(:,:,1));

y = zeros(size(w,1),size(w,2),size(x,3));
for l=1:size(x,3)
    y(:,:,l) = spatial(x(:,:,l));
end;

for r=1:size(y,1)
    for c=1:size(y,2)
        a = y(r,c,:);
        a = dct(a(:));
        y(r,c,:) = a(:);
    end;
end;

return;

function y = spatial(x)

[A,B,C,D] = dwt2(x,'sym8');
[AA,AB,AC,AD] = dwt2(A,'sym8');
[AAA,AAB,AAC,AAD] = dwt2(AA,'sym8');

AA =[AAA AAB; AAC AAD];

T = zeros(size(AA));
T(1:size(AB,1),1:size(AB,2)) = AB; AB = T;
T(1:size(AC,1),1:size(AC,2)) = AC; AC = T;
T(1:size(AD,1),1:size(AD,2)) = AD; AD = T;

A = [AA AB; AC AD];

T = zeros(size(A));
T(1:size(B,1),1:size(B,2)) = B; B = T;
T(1:size(C,1),1:size(C,2)) = C; C = T;
T(1:size(D,1),1:size(D,2)) = D; D = T;

y = [A B; C D];

return