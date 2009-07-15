<< data.m

data = Select[ data, Length[#] > 0 & ];

(* Normalize to seconds *)
data = Map[ Map[ { #[[1]] / 10^6, #[[2]] } &, # ] &, data ];

plot = ListPlot[ data, AxesOrigin->{data[[1,1,1]], 0}, PlotStyle->PointSize[Tiny] ];

Export[ "progress_trace.png", plot, ImageResolution->200 ];
