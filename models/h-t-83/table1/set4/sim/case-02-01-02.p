# petrisrvn 4.1
# petrisrvn -p  case-02-01-02.in
V y
C 9.9625e-06
I 44
PP 5
NP 1
#!User:  0:00:00.14
#!Sys:   0:00:00.12
#!Real:  0:00:00.48

W 12
  t1 : -1

:
     a1 d1 0.000000 -1
     a1 d2 0.000000 -1
     a1 d3 0.000000 -1
     a1 d4 0.000000 -1
     b1 d1 0.003125 -1
     b1 d2 0.003125 -1
     b1 d3 0.003125 -1
     b1 d4 0.003125 -1
     b2 d1 0.003125 -1
     b2 d2 0.003125 -1
     b2 d3 0.003125 -1
     b2 d4 0.003125 -1
  -1
-1


J 1
t1 : b2 b1 0.943792 0.000000
  -1
-1


X 5
t1 : e1 0.788804 -1
  -1

:
     a1 0.600000 -1
     a2 0.000000 -1
     b1 0.631246 -1
     b2 0.631246 -1
     c1 0.000000 -1
  -1
d1 : d1 0.040000 -1
  -1
d2 : d2 0.040000 -1
  -1
d3 : d3 0.040000 -1
  -1
d4 : d4 0.040000 -1
  -1
-1

FQ 5
t1 : e1 1.267742 1.000000 -1 1.000000
-1

:
    a1              1.267742 0.760645 -1
    a2              0.253566 0.000000 -1
    b1              0.253610 0.160090 -1
    b2              0.253610 0.160090 -1
    c1              0.253650 0.000000 -1
  -1
d1 : d1 4.437406 0.177496 -1 0.177496
-1
d2 : d2 4.437406 0.177496 -1 0.177496
-1
d3 : d3 4.437406 0.177496 -1 0.177496
-1
d4 : d4 4.437406 0.177496 -1 0.177496
-1
-1

P p1 1
t1 1 0 1 e1 0.354992 0.000000 -1 -1

:
         a1 0.253548 0 -1
         a2 0 0 -1
         b1 0.0507221 0 -1
         b2 0.0507221 0 -1
         c1 0 0 -1
 -1
-1

P d1 1
d1 1 0 1 d1 0.177496 0.000000 -1 -1
-1

P d2 1
d2 1 0 1 d2 0.177496 0.000000 -1 -1
-1

P d3 1
d3 1 0 1 d3 0.177496 0.000000 -1 -1
-1

P d4 1
d4 1 0 1 d4 0.177496 0.000000 -1 -1
-1
-1

