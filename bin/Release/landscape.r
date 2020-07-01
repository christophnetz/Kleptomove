library(tseries)
library(tidyverse)
library(dplyr)

capacity <- read.matrix("capacity_bitmap_v4_channel2.txt", sep="\t")
datameans <- data.frame(Group.1=numeric(),
                        items=numeric(),
                        foragers=numeric(),
                        klepts=numeric(),
                        replicate=character())

whichgen <- c("991","992","993","994","995","996","997", "998")
whichgen <- c("999")
type <- "facultative"
replicates <- c("rep1","rep2","rep3")

for(rep in replicates){
  


items <- read.matrix(paste0(whichgen[1], type,"_", rep,"items.txt"), sep="\t")
foragers <- read.matrix(paste0(whichgen[1], type,"_", rep,"foragers.txt"), sep="\t")
klepts <- read.matrix(paste0(whichgen[1], type,"_", rep,"klepts.txt"), sep="\t")
for(g in whichgen[2:length(whichgen)]){

  items <- items + read.matrix(paste0(g, type,"_", rep,"items.txt"), sep="\t")
  foragers <- foragers + read.matrix(paste0(g, type,"_", rep,"foragers.txt"), sep="\t")
  klepts <- klepts + read.matrix(paste0(g, type,"_", rep,"klepts.txt"), sep="\t")
  }




colnames(items) <- paste0("",seq(1,512))
rownames(items) <- paste0("",seq(1,512))
colnames(foragers) <- paste0("",seq(1,512))
rownames(foragers) <- paste0("",seq(1,512))
colnames(klepts) <- paste0("",seq(1,512))
rownames(klepts) <- paste0("",seq(1,512))
colnames(capacity) <- paste0("",seq(1,512))
rownames(capacity) <- paste0("",seq(1,512))

items %>% 
  as.data.frame() %>%
  rownames_to_column("f_id") %>%
  pivot_longer(-c(f_id), names_to = "samples", values_to = "items") %>%
  {. ->> ourdata }   #here is save
  
foragers %>% 
  as.data.frame() %>%
  rownames_to_column("f_id") %>%
  pivot_longer(-c(f_id), names_to = "samples", values_to = "foragers") %>%
  {. ->> temp_data }   #here is save

ourdata$foragers <- temp_data$foragers
klepts %>% 
  as.data.frame() %>%
  rownames_to_column("f_id") %>%
  pivot_longer(-c(f_id), names_to = "samples", values_to = "klepts") %>%
  {. ->> temp_data }   #here is save

ourdata$klepts <- temp_data$klepts

capacity %>% 
  as.data.frame() %>%
  rownames_to_column("f_id") %>%
  pivot_longer(-c(f_id), names_to = "samples", values_to = "capacity") %>%
  {. ->> temp_data }   #here is save

ourdata$capacity <- temp_data$capacity

ourdata$capacity <- floor(ourdata$capacity * 5)


ourdata$f_id <- as.character(ourdata$f_id)
#Then turn it back into a factor with the levels in the correct order
ourdata$f_id <- factor(ourdata$f_id, levels=unique(ourdata$f_id))



datameans1 <- aggregate(ourdata[,3:5], list(ourdata$capacity), mean)
datameans1$replicate <- rep
datameans <- rbind(datameans, datameans1[1:5,])
#datamedians<- aggregate(ourdata[,3:5], list(ourdata$capacity), median)
}


datameans$foragers <- datameans$foragers * 100
datameans$klepts <- datameans$klepts * 100
colnames(datameans)[c(1,3,4)] <- c("capacity","foragers*100", "klepts*100")



datameans2 <- melt(data = datameans, 
     measure.vars  = c("items", "foragers*100" ,"klepts*100"), 
     variable.name = "category", 
     value.name = "Density"
)

plot <- ggplot(datameans2, aes(x=capacity, y=Density, color=category, group=interaction(replicate, category), linetype=replicate))+
  geom_line()+
  scale_color_manual(values=c("green", "blue", "red"))+
  ggtitle(type)


ggsave(
  paste0(type, ".png"),
  plot,
  width = 8,
  height = 6.5,
  dpi=300
)

# 
# ggplot(data=ourdata, aes(x=factor(samples, level = unique(ourdata$samples)), y=f_id, fill=items)) + 
#   geom_raster() +
#   scale_fill_viridis_c(
#                        limits=c(0,10))+ scale_y_discrete(breaks=unique(x2$f_id)[seq(1,length(unique(x2$f_id)),by=32)]) +
#   scale_x_discrete(breaks=unique(x2$samples)[seq(1,length(unique(x2$samples)),by=32)])
# 
# 
# 
# 
# 
# ourdata <- ourdata[order(ourdata$capacity),]
# ourdata$rowid <- 1:length(ourdata$f_id)
# 
# plot1 <- ggplot(ourdata, aes(x=rowid, y=items))+
#   geom_point(colour="green")+
#   geom_point(aes(x=rowid, y=foragers), colour="blue")+
#   geom_point(aes(x=rowid, y=klepts), colour="red")
# 
# ourdata <- ourdata[order(ourdata$foragers),]
# ourdata$rowid2 <- 1:length(ourdata$f_id)
# 
# plot2 <- ggplot(ourdata, aes(x=rowid2, y=foragers))+
#   geom_point(colour="blue")+
#   geom_point(aes(x=rowid, y=klepts), colour="red", size=0.1)+
#   geom_point(aes(x=rowid, y=items), colour="green", size=0.1)
#   
# ggplot(ourdata, aes(x=capacity, y=foragers))+
#   geom_point(colour="blue")+
#   geom_point(aes(x=capacity, y=klepts), colour="red", size=0.1)


####### Reaction norms

library(plotly)
library(magrittr)

G <- generation(4000)
G$agents$ann

mcr <- function(x, n, drop = FALSE) { #'most common row' 
  xx <- do.call("paste", c(data.frame(x), sep = "\r")) 
  tx <- table(xx)
  sx = sort(tx,decreasing=TRUE)
  mx <- names(sx)[n[1]] 
  x[match(mx, xx), , drop = drop] 
}
mcann <- mcr(G$agents$ann, 3)

eq = function(i1, i2, i3){as.numeric((mcann[1,5] + i1*mcann[1,6] + i2*mcann[1,7] + i3*mcann[1,8]) >= 0)}
eq2 = function(i1, i2, i3){(mcann[1,5] + i1*mcann[1,6] + i2*mcann[1,7] + i3*mcann[1,8])}


x <- y <- z <- seq(0, 1, length.out = 50)
xyz <- mesh(x, y, z)
x <- as.vector(xyz$x)
y <- as.vector(xyz$y)
z <- as.vector(xyz$z)


fig <- plot_ly(
  type='isosurface',
  x = x,
  y = y,
  z = z,
  value = eq(x,y,z),
  isomin=0.5,
  isomax=2
)

fig <- fig %>% layout(
  title = config$outdir,
  scene = list(
    xaxis = list(title = "nonhandlers"),
    yaxis = list(title = "handlers"),
    zaxis = list(title = "items")
  ))


fig

