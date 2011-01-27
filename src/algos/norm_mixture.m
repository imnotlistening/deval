#!/usr/bin/octave -qf
##
## Generate a set of data that is composed of two normal distributions.
##

arg_list = argv();
if ( nargin != 6 )
  fprintf(stderr, "Usage: ./norm_mixture M1 D1 M2 D2 P samples\n");
  fprintf(stderr, "  Where M1 and M2 are the means\n");
  fprintf(stderr, "  and D1 and D2 are the deviations.\n");
  fprintf(stderr, "  and P is the probability of picking the first norm.\n");
  exit
endif

## Get our args now.
M1 = str2num(arg_list{1});
D1 = str2num(arg_list{2});
M2 = str2num(arg_list{3});
D2 = str2num(arg_list{4});
P = str2num(arg_list{5});
pts = str2num(arg_list{6});

for i = 1:pts
  dist = rand();
  if ( dist <= P )
    tmp = normrnd(M1, D1);
  else
    tmp = normrnd(M2, D2);
  endif

  printf("%f\n", tmp);

endfor
