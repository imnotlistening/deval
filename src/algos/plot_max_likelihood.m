# 
# Plot the passed normal distributions maximum likehood with various
# parameters. data is a vector containing the data for which the maximum
# likelihood is calculated against, mean is the range of values for the mean
# to use and stddev is the rande of values for the stddev to use. The result
# is a 3D mesh plot of the normal maximum likelihood across the mean/stddev.
#
function plot_max_likelihood ( data, mean, stddev )

  i_ind = 1;
  j_ind = 1;

  mesh_data = zeros(length(mean),length(stddev));

  for i = mean
    j_ind = 1;
    for j = stddev
      mesh_data(i_ind, j_ind) = sum(log(normpdf(data, i, j)));
      j_ind += 1;
    endfor
    i_ind += 1;
  endfor

  # Now we have some mesh data. Plot it.
  printf("length (mean) = %d\n", length(mean));
  printf("length (stddev) = %d\n", length(stddev));
  printf("size of mesh_data: ");
  size(mesh_data)
  mesh(stddev, mean, mesh_data);

endfunction