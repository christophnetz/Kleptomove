

args = commandArgs(trailingOnly=TRUE)

source(paste0(args, "\\sourceMe.r"))
config$dir = args # set dir explicit

#install.packages("lattice") 
#library(lattice)
require(ggplot2)
require(cowplot)
require(gridExtra)
require(png)
require(reshape2)

gens = config$G 
#fitmeans_P =  SMRY$pred[,1]
#fitmeans_H = SMRY$pred[,1]

# div_P = vector() analog to SMRY$pred[,4]?
# div_H = vector()

minwv = -1.01;     # minimal (weight) value
maxwv = 1.01;     # maximal (weight) value
steps = 101;  # num. of bins across range
stepsize = (maxwv - minwv)/steps  # bin range size



#Frequency matrix:

mtrxwP1 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP2 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP3 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP4 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP5 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP6 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP7 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix
mtrxwP8 <- matrix(nrow = steps, ncol = gens, dimnames=list(seq(minwv+stepsize, maxwv, stepsize)))  # Frequency matrix


 start.time <- Sys.time()
for (i in 1:gens){
  G = generation((i-1))
  
  
  #Weights
  mtrxwP2[,i] = table(cut(tanh(G$agents$ann[,2] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,2])
  mtrxwP3[,i] = table(cut(tanh(G$agents$ann[,3] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,3])
  mtrxwP4[,i] = table(cut(tanh(G$agents$ann[,4] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,4])
  mtrxwP5[,i] = table(cut(tanh(G$agents$ann[,5] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,1])
  if(config$agents.obligate==0){
      mtrxwP6[,i] = table(cut(tanh(G$agents$ann[,6] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,6])
      mtrxwP7[,i] = table(cut(tanh(G$agents$ann[,7] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,7])
      mtrxwP8[,i] = table(cut(tanh(G$agents$ann[,8] * 20), seq(minwv, maxwv, stepsize), right=T))/length(G$agents$ann[,8])
  
  }

  
}
 end.time <- Sys.time()
 time.taken <- end.time - start.time
 time.taken

#SMRY = summary()

P2P <- ggplot(data = melt(t(mtrxwP2[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[non-h]*' move')) + 
  geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                     values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
  theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")

P3P <- ggplot(data = melt(t(mtrxwP3[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[h]*' move')) + 
  geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                     values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
  theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
P4P <- ggplot(data = melt(t(mtrxwP4[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[i]*' move')) + 
  geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                     values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
  theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
P5P <- ggplot(data = melt(t(mtrxwP5[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[b]*' forage')) + 
  geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                     values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
  theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
if(config$agents.obligate==0){
  
  P6P <- ggplot(data = melt(t(mtrxwP6[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[non-h]*' forage')) + 
    geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                       values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
    theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
  
  P7P <- ggplot(data = melt(t(mtrxwP7[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[h]*' forage')) + 
    geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                       values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
    theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
  
  P8P <- ggplot(data = melt(t(mtrxwP8[,1:gens])), aes(x=Var1, y=Var2, fill=value)) + labs(x="", y=expression('W'[i]*' forage')) + 
    geom_tile() + scale_fill_gradientn(colours = colorRampPalette(c("white", "red", "blue"))(3), 
                                       values = c(0, 0.3 , 1), space = "Lab", guide = FALSE) + geom_hline(yintercept = 0)+ theme_bw() +
    theme(axis.title.x=element_text(size=14), axis.title.y=element_text(size=12), panel.grid.major = element_blank(), panel.grid.minor = element_blank(),axis.line = element_line(colour = "black"), legend.position = "none")
} 

if(config$agents.obligate==1){
    weightplots <- plot_grid(P2P,P3P,P4P,P5P, nrow = 4, align = "h", label_size = 16)
    } 
if(config$agents.obligate==0) {
    weightplots <- plot_grid(P2P,P3P,P4P,P5P,P6P,P7P,P8P, nrow = 4, align = "h", label_size = 16)
  }
ggsave(
  paste0(config$outdir, "_weigths.png"),
  weightplots,
  width = 6,
  height = 6,
  dpi=150
)
