#This script is intended to analyze the changing in the ANN, during the generations

x <- list()
g <- seq(0,500,10)
for (i in 1:length(g)){
  x[i] <- generation(g[i])
}
str(x)

library(tidyverse)
y <- list()
for (i in 1:length(x)){
  y[[i]] <- tibble(
    gen = c(rep(g[i],length(x[[i]]$fit))),
    id = c(1:length(x[[i]]$fit)),
    w1 = x[[i]]$ann[,1],
    w2 = x[[i]]$ann[,2],
    w3 = x[[i]]$ann[,3],
    w4 = x[[i]]$ann[,4],
    w5 = x[[i]]$ann[,5],
    w6 = x[[i]]$ann[,6],
    w7 = x[[i]]$ann[,7],
    w8 = x[[i]]$ann[,8])}

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#Looking to make sense of a single one

# a <- y[[8]]
# a
# 
library(reshape2)
# df.m <- melt(a,"id")
# 
# ggplot(df.m, aes(x = value,y=1))+
#   geom_point(alpha=1/100)+
#   facet_wrap(~variable, scales = "free")


##############################
#reshaping but not with reshape2

z <- dplyr::bind_rows(y)

a <- z %>% 
  gather(w1,w2,w3,w4,w5,w6,w7,w8, key="node", value="weight")
a

b <- a %>% 
  mutate(bin=plyr::round_any(weight,0.01)) %>% 
  group_by(gen,node,bin) %>% 
  summarize(count=n())
b

ggplot(b,aes(x=gen, y=bin, fill=count))+
  geom_tile()+
  theme_bw()+
  facet_wrap(~node, scale="free")+
  ylim(-0.5,0.5)+
  scale_fill_gradient2(low="white",high = "blue",mid = "red", midpoint = 2500, limit=c(0,10000))
  #scale_fill_viridis_c(direction = -1)
  #scale_fill_distiller(palette="Blues", direction = 1)
  
#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#for checking how many individuals are being discarded in the plot:
test <- arrange(b, desc(bin))
View(test)
#the frist n rows and the last m rows that are outside the limists in the plot, are not represented in the plot
#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

####################################
#This code is taken from something christoph wrote, and does pretty much the same as before, but print each node-plot separately
g <- seq(0,500,10)#Vthis needs to be changed if you want to analyze more generations or in more detail
gc <- length(g)

minwv = -1.01;     # minimal (weight) value
maxwv = 1.01;     # maximal (weight) value
steps = 101;  # num. of bins across range
stepsize = (maxwv - minwv)/steps  # bin range size


mtrxwP1 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP2 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP3 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP4 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix

mtrxwH1 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwH2 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwH3 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwH4 <- matrix(nrow = steps, ncol = gc, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix

for(i in 1:gc){
  x=generation(i-1)
  
  #Weights
  mtrxwP1[,i] = table(cut(tanh(x$agents$ann[,1] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,1])
  mtrxwP2[,i] = table(cut(tanh(x$agents$ann[,2] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,2])
  mtrxwP3[,i] = table(cut(tanh(x$agents$ann[,3] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,3])
  mtrxwP4[,i] = table(cut(tanh(x$agents$ann[,4] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,4])
  
  
  mtrxwH1[,i] = table(cut(tanh(x$agents$ann[,5] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,5])
  mtrxwH2[,i] = table(cut(tanh(x$agents$ann[,6] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,6])
  mtrxwH3[,i] = table(cut(tanh(x$agents$ann[,7] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,7])
  mtrxwH4[,i] = table(cut(tanh(x$agents$ann[,8] * 20), seq(minwv, maxwv, stepsize), right=T))/length(x$agents$ann[,8])
}

GRA1 <- ggplot(data = melt(t(mtrxwP1)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA2 <- ggplot(data = melt(t(mtrxwP2)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA3 <- ggplot(data = melt(t(mtrxwP3)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA4 <- ggplot(data = melt(t(mtrxwP4)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA5 <- ggplot(data = melt(t(mtrxwH1)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA6 <- ggplot(data = melt(t(mtrxwH2)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA7 <- ggplot(data = melt(t(mtrxwH3)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
GRA8 <- ggplot(data = melt(t(mtrxwH4)), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[H]*' Herbivore')) +
  geom_tile() + scale_fill_gradient2(low = "white", high = "blue", mid = "red", 
                                     midpoint = 0.5, limit = c(0,1), space = "Lab", 
                                     name="Frequency", guide = FALSE) + geom_hline(yintercept = 0)+ theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))

# GRA1
# GRA2
# GRA3
# GRA4
# GRA5
# GRA6
# GRA7
# GRA8

library(ggpubr)
theme_set(theme_pubr())
figure <- ggarrange(GRA1,GRA3,GRA3,GRA4,GRA5,GRA6,GRA7,GRA8,
                    labels = c("W1","W3","W3","W4","W5","W6","W7","W8"),
                    ncol = 4,nrow = 2)
figure
