library(ggplot2)
library(grid)

vp.layout <- function(x, y) viewport(layout.pos.row=x, layout.pos.col=y)
arrange <- function(..., nrow=NULL, ncol=NULL, as.table=FALSE) {
 dots <- list(...)
 n <- length(dots)
 if(is.null(nrow) & is.null(ncol)) { nrow = floor(n/2) ; ncol = ceiling(n/nrow)}
 if(is.null(nrow)) { nrow = ceiling(n/ncol)}
 if(is.null(ncol)) { ncol = ceiling(n/nrow)}
        ## NOTE see n2mfrow in grDevices for possible alternative
grid.newpage()
pushViewport(viewport(layout=grid.layout(nrow,ncol) ) )
 ii.p <- 1
 for(ii.row in seq(1, nrow)){
 ii.table.row <- ii.row 
 if(as.table) {ii.table.row <- nrow - ii.table.row + 1}
  for(ii.col in seq(1, ncol)){
   ii.table <- ii.p
   if(ii.p > n) break
   print(dots[[ii.table]], vp=vp.layout(ii.table.row, ii.col))
   ii.p <- ii.p + 1
  }
 }
}

t <- read.table("<<datafile>>", header=TRUE, colClasses=c(Date="character"))
t$DT <- paste(t$Date,t$Time)
t$DateTime <- as.POSIXct(strptime(t$DT, '%m%d %H:%M:%S.%OS'))

png("<<imagefile>>_summary.png",width=1800,height=1200)

p <- qplot(DateTime,RTT, data=t, geom = c("point", "smooth"), ylim=c(0, 0.5),
span = 0.2)
ggsave(filename="<<imagefile>>rtt_detail.png", plot=p)

p2 <- qplot(DateTime,Jitter, data=t, geom = c("point", "smooth"),
span = 0.2)
ggsave(filename="<<imagefile>>jitter_detail.png", plot=p2)

p3 <- qplot(DateTime,Loss, data=t, geom = c("point", "smooth"),
span = 0.2)
ggsave(filename="<<imagefile>>loss_detail.png", plot=p3)

p4 <- qplot(DateTime,LossFraction, data=t, geom = c("point", "smooth"),
span = 0.2)
ggsave(filename="<<imagefile>>lf_detail.png", plot=p4)

# Arrange and display the plots into a 2x1 grid
arrange(p,p2,p3,p4,ncol=1)
dev.off()

