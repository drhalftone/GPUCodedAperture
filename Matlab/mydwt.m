function y = mydwt(x)

LoD = [  -0.00338242,  -0.00054213,   0.03169509,   0.00760749,  -0.14329424,  -0.06127336,   0.48135965,   0.77718575,   0.36444189,  -0.05194584,  -0.02721903,   0.04913718,   0.00380875,  -0.01495226,  -0.00030292,   0.00188995 ];
HiD = [  -0.00188995,  -0.00030292,   0.01495226,   0.00380875,  -0.04913718,  -0.02721903,   0.05194584,   0.36444189,  -0.77718575,   0.48135965,   0.06127336,  -0.14329424,  -0.00760749,   0.03169509,   0.00054213,  -0.00338242 ];

LoD = fliplr(LoD);
HiD = fliplr(HiD);

a = imfilter(x, LoD, 'full');  a = a(:,1:2:end,:);
b = imfilter(x, HiD, 'full');  b = b(:,1:2:end,:);

A = imfilter(a, LoD', 'full'); A = A(1:2:end,:,:);
B = imfilter(a, HiD', 'full'); B = B(1:2:end,:,:);
C = imfilter(b, LoD', 'full'); C = C(1:2:end,:,:);
D = imfilter(b, HiD', 'full'); D = D(1:2:end,:,:);

c = imfilter(A, LoD, 'full');  c = c(:,1:2:end,:);
d = imfilter(A, HiD, 'full');  d = d(:,1:2:end,:);

AA = imfilter(c, LoD', 'full'); AA = AA(1:2:end,:,:);
AB = imfilter(c, HiD', 'full'); AB = AB(1:2:end,:,:);
AC = imfilter(d, LoD', 'full'); AC = AC(1:2:end,:,:);
AD = imfilter(d, HiD', 'full'); AD = AD(1:2:end,:,:);

e = imfilter(AA, LoD, 'full');  e = e(:,1:2:end,:);
f = imfilter(AA, HiD, 'full');  f = f(:,1:2:end,:);

AAA = imfilter(e, LoD', 'full'); AAA = AAA(1:2:end,:,:);
AAB = imfilter(e, HiD', 'full'); AAB = AAB(1:2:end,:,:);
AAC = imfilter(f, LoD', 'full'); AAC = AAC(1:2:end,:,:);
AAD = imfilter(f, HiD', 'full'); AAD = AAD(1:2:end,:,:);

s = 2*size(AAA) + size(AA) + size(A);
y = zeros(s(1), s(2), size(x,3));

s = size(AAA);
y(1:s(1),1:s(2),:) = AAA;
y(1:s(1),s(2)+1:2*s(2),:) = AAB;
y(s(1)+1:2*s(1),1:s(2),:) = AAC;
y(s(1)+1:2*s(1),s(2)+1:2*s(2),:) = AAD;
s = 2*s;

r = size(AA);
y(1:r(1),s(2)+1:s(2)+r(2),:) = AB;
y(s(1)+1:s(1)+r(1),1:r(2),:) = AC;
y(s(1)+1:s(1)+r(1),s(2)+1:s(2)+r(2),:) = AD;

s = s+r;
r = size(A);
y(1:r(1),s(2)+1:s(2)+r(2),:) = B;
y(s(1)+1:s(1)+r(1),1:r(2),:) = C;
y(s(1)+1:s(1)+r(1),s(2)+1:s(2)+r(2),:) = D;

return;
