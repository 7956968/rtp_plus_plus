library(ggplot2)
library(grid)

plot_packets <- function(t, y_src, y_dst){
  rows <- dim(t)[1]
  # sender to receiver
  for (row in 1:rows)
  {
    flow_id <- t$FID[row]
    if (flow_id == "0")
    {
      if (t$OWD[row] != "-1")
      {
        arrows(t$Time[row], y_src, t$Arrival[row], y_dst, col="red")
      }
    }
    else
    {
      if (t$OWD[row] != "-1")
      {
        arrows(t$Time[row], y_src, t$Arrival[row], y_dst, col="green")
      }
    }
  }
}

plot_path_specific_trace <- function(file_ps_est, y_src, y_dst, color, limit=0)
{
  # path specific
  t_est <- read.table(file_ps_est, header=TRUE)
  if (limit > 0)
    t_est <- head(t_est, limit)
  
  rows_est <- dim(t_est)[1]
  
  for (row in 1:rows_est)
  {
    arrows(t_est$Time[row], y_src, t_est$Time[row] + t_est$DELAY[row], y_dst, col=color, lty=2)
  }
}

# plots short and long paths
plot_crosspath_trace <- function(file_cp_est, y_src, y_dst, color_short, color_long, color_lost, color_early_detect)
{
  ## cross path
  t_est <- read.table(file_cp_est, header=TRUE)
  t_est$STO <- t_est$Time + t_est$SHORT
  t_est$LTO <- t_est$Time + t_est$LONG
  rows_est <- dim(t_est)[1]
  
  for (row in 1:rows_est)
  {
    if (t_est$LONG_USED[row] == 1)
    {
      arrows(t_est$Time[row], y_src, t_est$LTO[row], y_dst, col=color_long, lty=2)
    }
    
    arrows(t_est$Time[row], y_src, t_est$STO[row], y_dst, col=color_short, lty=2)
    
#     if (t_est$EARLY[row] != "-1")
#     {
#       arrows(t_est$Time[row] + t_est$SHORT[row], y_src, t_est$Time[row] + t_est$EARLY[row], y_dst, col=color_early_detect, lty=2)
#     }
  }
}

# plots short or long paths
plot_crosspath_trace2 <- function(file_cp_est, y_src, y_dst, color_short, color_long, color_lost, color_early_detect, limit=0)
{
  ## cross path
  t_est <- read.table(file_cp_est, header=TRUE)
  if (limit > 0)
    t_est <- head(t_est, limit)
  
  t_est$STO <- t_est$Time + t_est$SHORT
  t_est$LTO <- t_est$Time + t_est$LONG
  rows_est <- dim(t_est)[1]
  
  for (row in 1:rows_est)
  {
    if (t_est$LONG_USED[row] == 0)
    {
      arrows(t_est$Time[row], y_src, t_est$STO[row], y_dst, col=color_short, lty=2)  
    }
    else
    {
      arrows(t_est$Time[row], y_src, t_est$LTO[row], y_dst, col=color_long, lty=2)
    }
    
#     if (t_est$LONG2[row] != "-1")
#     {
#       # hack to not draw lines between packets on time
#       if ( t_est$LONG2[row] > 5)
#       {
#         arrows(t_est$Time[row], y_src, t_est$LTO2[row] + t_est$SHORT[row], y_dst, col=color_long, lty=2)
#       }
#     }
#      else
#     {
#       arrows(t_est$Time[row], y_src, t_est$STO[row], y_dst, col=color_short, lty=2)  
#     }
#     
#     if (t_est$EARLY[row] != "-1")
#     {
#       arrows(t_est$Time[row] + t_est$SHORT[row], y_src, t_est$Time[row] + t_est$EARLY[row], y_dst, col=color_early_detect, lty=2)
#     }
  }
}

# this function plots the packets sent from sender to receiver
plot_packet_trace <- function(file, height = 15, mirror=TRUE, limit=0, min_x=0, max_x=0){
  # read data
  t <- read.table(file, header=TRUE)
  if (limit > 0)
    t <- head(t, limit)

  border = 50
  # calc arrival time
  t$Arrival <- t$Time + t$OWD
  
  if (min_x > 0)
  {
    t <- subset(t, t$Time > min_x )
    min_x <- min_x - border
  }
  else
  {
    min_x <- min(t$Arrival) - border
  }
  
  if (max_x > 0)
  {
    t <- subset(t, t$Arrival < max_x )
    max_x <- max_x + border
  }
  else
  {
    max_x <- max(t$Arrival) + border
  }
    
  
  plot.new()
  plot(0,ylab='',xlab='', xlim=c(min_x,max_x), ylim=c(0,height))
  
  # reference lines
  x1 <- c(0, max_x)
  y1 <- c(height, height)
  y_sender <- height
  if (mirror == FALSE)
  {
    y_receiver <- height/2
    y2 <- c(y_receiver, y_receiver)
    y3 <- c(0, 0)
    # sender line
    lines(x1, y1)  
    # receiver line
    lines(x1, y2)  
    # bottom border
    lines(x1, y3)  
  }
  else
  {
    y_receiver <- 2*height/3
    y2 <- c(y_receiver, y_receiver) 
    # sender line
    lines(x1, y1)  
    # receiver line
    lines(x1, y2)  
    y_receiver2 <- height/3
    y_sender2 <- 0
    y3 <- c(y_receiver2, y_receiver2)
    y4 <- c(y_sender2, y_sender2)
    # receiver line
    lines(x1, y3)
    # sender line
    lines(x1, y4)
  }
  
  plot_packets(t, y_sender, y_receiver)
  
  if (mirror == TRUE)
  {
    y_receiver2 <- height/3
    plot_packets(t, 0.0, y_receiver2)
  }
}

plot_stat_summary <-(file_summary)
{
  t_est <- read.table(file_summary, header=TRUE)
  t$PS_WAIT
}