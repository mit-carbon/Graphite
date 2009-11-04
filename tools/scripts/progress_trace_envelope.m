<< ~/research/graphite/src2/output_files/data.m

data = Select[ data, Length[#] > 0 & ];

(* Normalize to seconds *)
data = Map[ Map[ { #[[1]] / 10^6, #[[2]] } &, # ] &, data ];

If[Length[data] == 0, Print["ERROR: No data to plot."]; Exit[]]

(* Split list into bins *)

maxTime = data[[1,-1,1]];
numPlotPoints = 1000;
timeInterval = maxTime / numPlotPoints;
quotientF[x_] = Quotient[ #[[1]], x ] &;
splitF = GatherBy[ #, quotientF[ timeInterval ] ] &;
data = Flatten[ data, 1 ]; (* Join into a single list *)
bins = splitF[ data ]; (* Split into bins *)

(* Find statistics *)

binsSim = Map[ #[[2]] &, bins, {2} ]; (* Get rid of real-time *)
mins = Map[ Min, binsSim ];
maxs = Map[ Max, binsSim ];
means = Map[ Mean, binsSim ];
minDiff = mins - means;
maxDiff = maxs - means;

binsReal = Map[ #[[1]] &, bins, {2} ]; (* Get rid of sim-time *)
times = Map[ Mean, binsReal ];

(* Plot *)
minPlot = Table[ { times[[i]], minDiff[[i]] }, {i, Length[times]} ] // Sort;
maxPlot = Table[ { times[[i]], maxDiff[[i]] }, {i, Length[times]} ] // Sort;

plot = ListPlot[ {minPlot, maxPlot}, Joined->True ];

(*
plotStyles = Array[ AbsolutePointSize[ If[ # == 1, 1.0, 1.0] ] &, Length[data] ];

Set::write: Tag Times in 90 plotStyles is Protected.

plot = ListPlot[ data, AxesOrigin->{data[[1,1,1]], 0}, PlotStyle->plotStyles,
               Frame->{{True,False},{True,False}},
               FrameLabel->{"Run-time (secs)","Cycles (simulated)"},
               Axes->False,
               BaseStyle->{FontSize->16}];
*)

Export[ "progress_trace_envelope.png", plot, ImageResolution->200 ];
