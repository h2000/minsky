example=argv(){1};
source ([example ".m"]);
source ([example ".dat"]);

# split input data into timestep vector and data to compare
t=d(:,1);
xc=d(:,2:size(d,2));
x=lsode(@f, x0, t);


if (any(x-xc > 0.01*(abs(x)+abs(xc))))
  [i,j]=find(x-xc > 0.001*(abs(x)+abs(xc)));
  simulation_differs_at_t=t(unique(i))
  exit(1)
endif
