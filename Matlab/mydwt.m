function x = mydwt(x)

LoD = [  -0.00338242,  -0.00054213,   0.03169509,   0.00760749,  -0.14329424,  -0.06127336,   0.48135965,   0.77718575,   0.36444189,  -0.05194584,  -0.02721903,   0.04913718,   0.00380875,  -0.01495226,  -0.00030292,   0.00188995 ];
HiD = [  -0.00188995,  -0.00030292,   0.01495226,   0.00380875,  -0.04913718,  -0.02721903,   0.05194584,   0.36444189,  -0.77718575,   0.48135965,   0.06127336,  -0.14329424,  -0.00760749,   0.03169509,   0.00054213,  -0.00338242 ];

A = imfilter(x, LoD);  A = A(:,1:2:end,:);
B = imfilter(x, HiD);  B = B(:,1:2:end,:);
x = cat(2, A, B);

A = imfilter(x, LoD'); A = A(1:2:end,:,:);
B = imfilter(x, HiD'); B = B(1:2:end,:,:);
x = cat(1, A, B);

y = x(1:end/2,1:end/2,:);

A = imfilter(y, LoD);  A = A(:,1:2:end,:);
B = imfilter(y, HiD);  B = B(:,1:2:end,:);
y = cat(2, A, B);

A = imfilter(y, LoD'); A = A(1:2:end,:,:);
B = imfilter(y, HiD'); B = B(1:2:end,:,:);
y = cat(1, A, B);

z = y(1:end/2,1:end/2,:);

A = imfilter(z, LoD);  A = A(:,1:2:end,:);
B = imfilter(z, HiD);  B = B(:,1:2:end,:);
z = cat(2, A, B);

A = imfilter(z, LoD'); A = A(1:2:end,:,:);
B = imfilter(z, HiD'); B = B(1:2:end,:,:);
z = cat(1, A, B);

y(1:end/2,1:end/2,:) = z;
x(1:end/2,1:end/2,:) = y;

return;
