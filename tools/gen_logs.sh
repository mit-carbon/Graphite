cat output_files/app_* | sort -g | awk 'BEGIN {FS=OFS=" "} {$1 = ""} {print NR "\t" $0}' > output_files/log_app
cat output_files/sim_* | sort -g | awk 'BEGIN {FS=OFS=" "} {$1 = ""} {print NR "\t" $0}' > output_files/log_sim
cat output_files/sim_* output_files/app_* output_files/system | sort -g | awk 'BEGIN {FS=OFS=" "} {$1 = ""} {print NR "\t" $0}' > output_files/log_all
