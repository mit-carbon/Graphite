<< data.m

data = Select[ data, Length[#] > 0 & ];

(* data = Map[ { #[[1]] / 10^6, #[[2]] } &, data ]; *)

plot = ListLinePlot[ data, AxesOrigin->{data[[1,1,1]], Automatic} ];

Export[ "progress_trace.png", plot, ImageResolution->200 ];
