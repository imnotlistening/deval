#!/usr/bin/octave -qf

arg_list = argv();
if ( nargin != 3 )
  fprintf(stderr, "usage: ./norm.m <mu> <sigma> <samples>\n");
  exit
endif

mu      = str2num(arg_list{1});
sigma   = str2num(arg_list{2});
samples = str2num(arg_list{3});

data = normrnd(mu, sigma, 1, samples);

for i = 1:length(data)
  printf("%f\n", data(i));
endfor


