// 等边三角形
lc = 0.1;

Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {0.5, 0.866, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 1};

Curve Loop(1) = {1, 2, 3};

Plane Surface(1) = {1};