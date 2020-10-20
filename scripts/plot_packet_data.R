setwd("~/Projects/RLCD/rtp++/bin")

source("~/Projects/RLCD/rtp++/scripts/plot_packets_rto.R")

file <- "temp.txt"
file <- "mprtp_data_rr.txt"
file_cp_est <- "mprtp_cp_est_data.txt"
file_ps_est_0 <- "mprtp_ps_est_data_0.txt"
file_ps_est_1 <- "mprtp_ps_est_data_1.txt"

lim <- 0
lim <- 30
plot_packet_trace(file, mirror=FALSE, limit=lim)
plot_path_specific_trace(file_ps_est_0, 7.5, 0, "red", lim=lim)
plot_path_specific_trace(file_ps_est_1, 7.5, 0, "green", limit=lim)
plot_crosspath_trace(file_cp_est, 7.5, 0, "blue", "lightskyblue", "red", "orange")

lim <- 100
plot_packet_trace(file, mirror=TRUE)
plot_packet_trace(file, mirror=TRUE, max_x=100)
plot_packet_trace(file, mirror=TRUE, min_x=1000, max_x=3000)
plot_path_specific_trace(file_ps_est_0, 10, 5, "red")
plot_path_specific_trace(file_ps_est_1, 10, 5, "green")
plot_crosspath_trace(file_cp_est, 10, 5, "blue", "lightskyblue", "red", "orange")
plot_crosspath_trace2(file_cp_est, 10, 5, "blue", "lightskyblue", "red", "orange")

file_summary <- "mprtp_stats_summary.txt"
t_stats <- read.table(file_summary, header=TRUE)
ps_wait <- t_stats$PS_WAIT
ps_wait <- ps_wait[ps_wait > 0]
plot(ps_wait)
plot(ecdf(ps_wait))
plot(t_stats$PS_ERR)

t_ps_0 <- subset(t_stats, t_stats$ID == "0")
ps_0_wait <- t_ps_0$PS_WAIT
ps_0_wait <- ps_0_wait[ps_0_wait > 0]
plot(ps_0_wait)
plot(ecdf(ps_0_wait))
plot(t_ps_0$PS_ERR)

t_ps_1 <- subset(t_stats, t_stats$ID == "1")
ps_1_wait <- t_ps_1$PS_WAIT
ps_1_wait <- ps_1_wait[ps_1_wait > 0]
plot(ps_1_wait)
plot(ecdf(ps_1_wait))
plot(t_ps_1$PS_ERR)


cp_wait <- t_stats$CP_WAIT
cp_wait <- cp_wait[cp_wait > 0]
plot(cp_wait)
plot(ecdf(cp_wait))
plot(t_stats$CP_ERR)
plot(density(t_stats$CP_ERR))
