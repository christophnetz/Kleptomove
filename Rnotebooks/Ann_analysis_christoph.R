#Plotting all Ann, ma con il metodo di chirstoph che permette di visualizzare tutti i valori, 
#anche quelli distanti perchè usa il metodo tanh
getwd()
setwd("C:/Users/matteo/Desktop/OUTPUT_ANALYSIS/modelOutput_klepts_20_09_19")   #CHANGE THIS FOR MANUALLY ASSIGNING THE PATH

#list all folders that contain the word "sim"
lfolders <- list.dirs(recursive = FALSE)
folders <- lfolders[ grepl("sim", lfolders) ] #this is to eliminate from the listing every folder not containing "sim"
#folders

#calling libraries
library(data.table)
library(tidyverse)
library(ggpubr)

#How precise do you want the visualization of the weights?
g <- seq(0,990,10)			#THIS IS VERY IMPORTANT! WHATCH OUT HOW BIG YOU WANT THIS
g_len <- length(g)

val_min = -1.01;       # minimal (weight) value
val_max = 1.01;        # maximal (weight) value
s = 101;                      # num. of bins across range
s_size = (val_max - val_min)/s  # bin range size

figure_list <- list()            #ggarrange list
ann_weights_names_list <- list("w1_score_BIAS", "w2_score_nonHandlers", "w3_score_Handlers", "w4_score_Food", "w5_behav_BIAS", "w6_behav_nonHandlers", "w7_behav_Handlers", "w8_behav_Food")
parameters <- list()
#--------------------------------------------------------------------------------------------------------------
for(j in seq_along(folders)){
  source(paste(folders[[j]], "sourceMe.R", sep = "/"))
  
  #filling the title list
  parameters[[j]] <- c(paste("Simulation_", str_sub(config$outdir, start = 4), sep=""), config$agents.handling_time, config$landscape.item_growth)
  
  #empty frequency matrix
  w1 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix 
  w2 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix 
  w3 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  w4 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  
  w5 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  w6 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  w7 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  w8 <- matrix(nrow = s, ncol = g_len, dimnames=list(seq(val_min+s_size, val_max, s_size), g))  # Frequency matrix
  
  for(i in 1:g_len){
    gen_x = generation(g[i])
    scal_v = 2          #scaling value (I think the best scaling value is "2")
    
    #Weights
    w1[,i] = table(cut(tanh(gen_x$agents$ann[,1] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,1])
    w2[,i] = table(cut(tanh(gen_x$agents$ann[,2] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,2])
    w3[,i] = table(cut(tanh(gen_x$agents$ann[,3] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,3])
    w4[,i] = table(cut(tanh(gen_x$agents$ann[,4] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,4])
    
    
    w5[,i] = table(cut(tanh(gen_x$agents$ann[,5] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,5])
    w6[,i] = table(cut(tanh(gen_x$agents$ann[,6] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,6])
    w7[,i] = table(cut(tanh(gen_x$agents$ann[,7] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,7])
    w8[,i] = table(cut(tanh(gen_x$agents$ann[,8] * scal_v), seq(val_min, val_max, s_size), right=T)) / length(gen_x$agents$ann[,8])
  }
  
  ann_weights_list <- list(w1,w2,w3,w4,w5,w6,w7,w8)
  weights_plotlist <- list()
  
  for(i in 1:length(ann_weights_list)){
    
    weights_plotlist[[i]] <- ggplot(data = melt(t(ann_weights_list[[i]])), aes(x=Var1, y=Var2, fill=value)) +
      labs(title = ann_weights_names_list[[i]] ,x= "generations", y = paste("w",i,sep = "")) +
      geom_tile() +
      scale_fill_gradient2(low = "white", high = "blue", mid = "red", midpoint = 0.5, limit = c(0,1), space = "Lab", name="Frequency", guide = "legend") +
      #geom_hline(yintercept = 0, colour="gainsboro") + 
      theme(axis.title=element_text(size=15), axis.text = element_text(size = 10))
    
  }
  
  grafico <- ggarrange(plotlist = weights_plotlist, ncol = 4, nrow = 2, common.legend = TRUE, legend = "right")
  
  grafico_comp <- annotate_figure(grafico,
                                  top = text_grob(parameters[[j]][[1]], face = "bold", size = 20),
                                  fig.lab = paste("handling_", parameters[[j]][[2]]," / ", "growth_", parameters[[j]][[3]], sep = ""))
  figure_list[[j]] <- grafico_comp
}

# Outside Folderrs cycle ------------------------------------------------------
#print singular pdf for each one 
# for (i in seq_along(folders)){
#   nomefile=paste("simulation", i,".pdf", sep = "" )
#   ggexport(figure_list[[i]], filename = nomefile, width = 22, height = 8)
# }

#print a pdf with all the simulations
pdf(file = "Tanh_allSim2_1000.pdf", width = 19, height = 8 )
figure_list
dev.off()
