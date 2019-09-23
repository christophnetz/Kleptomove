#Author: Matteo Pederboni [Last modified 13/09/2019]
#This script allows you to plot all the simulations in the source folder (which must be manually inserted below)
#Plots of all the simulations are printed in a single pdf, in different pages.
#It is possible to change the visualization modifing the lines with this placeholder comment: [" §§§ "] 

getwd()
setwd("C:/Users/matteo/Desktop/OUTPUT_ANALYSIS/21-08-2019_k")   #CHANGE THIS FOR MANUALLY ASSIGNING THE PATH

#list all folders that contain the word "sim"
lfolders <- list.dirs(recursive = FALSE)
folders <- lfolders[ grepl("sim", lfolders) ] #this is to eliminate from the listing every folder not containing "sim"
#folders

#calling libraries
library(data.table)
library(tidyverse)
library(ggpubr)

#How precise do you want the visualization of the weights?
g <- seq(0,500,10)			#THIS IS VERY IMPORTANT! WHATCH OUT HOW BIG YOU WANT THIS
sim_ann_list <- list() 
parameters <- list()
# MyTheme -----------------------------------------------------------------------------------
my_theme_facet <- theme(plot.title = element_text(size=rel(1.5)),
                        panel.grid.major = element_line(color = "grey90"), 
                        panel.background = element_rect(fill = "white", colour = "grey70"),
                        axis.line = element_line(size = 0.5, colour = "black"),
                        strip.background = element_rect(colour = "black", fill = "black", size=1),
                        strip.text = element_text(colour = "white", face = "bold"))

# Cycle through folders ------------------------------------------------------------------------
for(i in seq_along(folders)){
  source(paste(folders[[i]], "sourceMe.R", sep = "/"))
  
  #filling the title list
  parameters[[i]] <- c(str_sub(config$outdir, start = 4), config$agents.handling_time, config$landscape.item_growth)
  
  #creating an empty list
  list_gen <- list()
  #filling it with the generations
  for (j in 1:length(g)){
    temp <- generation(g[j])
    list_gen[[j]] <- temp$agents$ann
  }
  
  #creating an empty ann_list
  ann_list <- list()
  
  #filling the ann_list
  for (j in 1:length(list_gen)){
    ann_list[[j]] <- tibble(
      gen =                     g[j], #maybe substitute with only g[j]
      id =                      c(1:dim(list_gen[[j]])[1]),
      w1_score_BIAS =           list_gen[[j]][,1],  #remove $ann
      w2_score_nonHandlers =    list_gen[[j]][,2],
      w3_score_Handlers =       list_gen[[j]][,3],
      w4_score_Food =           list_gen[[j]][,4],
      w5_behav_BIAS =           list_gen[[j]][,5],
      w6_behav_nonHandlers =    list_gen[[j]][,6],
      w7_behav_Handlers =       list_gen[[j]][,7],
      w8_behav_Food =           list_gen[[j]][,8])
  }
  
  #reshaping the list for creating a single dataframe
  ann_df <- dplyr::bind_rows(ann_list) %>% 
    mutate(simulation = str_sub(config$outdir, start = 4))
  
  #gathering all the weights
  ann0 <- ann_df %>% 
    gather(w1_score_BIAS, w2_score_nonHandlers, w3_score_Handlers, w4_score_Food, w5_behav_BIAS, w6_behav_nonHandlers, w7_behav_Handlers, w8_behav_Food, key="node", value="weight")
  
  #manipulating the dataframe
  ann1 <- ann0 %>% 
    mutate(bin=plyr::round_any(weight,0.01)) %>%           #["§§§"] you change the bin-range for a more or less distinct grouping
    group_by(gen,node,bin) %>% 
    summarize(count=n())
  
  sim_ann_list[[i]] <- ann1
}
# END Cycle though folders ----------------------------------------------------------------------------

#empty list of plots
ann_plot_list <- list()

#creating the plots and filling the list
for (i in 1:length(sim_ann_list)){
  
	titl_name <- paste("Simulation", parameters[[i]][1], sep = "_" )
	stitl_name <- paste("handling", parameters[[i]][2],"/ growth_rate", parameters[[i]][3])
	
	ann_plot_list[[i]] <- ggplot(sim_ann_list[[i]], aes(x=gen, y=bin, fill=count))+
	  geom_tile()+
	  labs(title = titl_name, x="Generations", y=NULL, subtitle = stitl_name)+
	  guides(fill=guide_legend(title="Agents"))+
	  facet_wrap(~node, scale="free", nrow = 2, ncol=4)+
	  scale_x_continuous(breaks = seq(0,1000,50), minor_breaks = NULL)+
	  scale_y_continuous(breaks = seq(-1.6, 1.6, 0.2), minor_breaks = waiver(), limits = c(-1.0,1.0))+                         #["§§§"] change the limits for zooming IN or OUT
	  #scale_fill_gradient2(low="white",high = "blue",mid = "red", midpoint = 2500, limit=c(0,10000))+                         #["§§§"] choose one of the commented scale_fill for different colors
	  scale_fill_gradient2(low="white",high = "firebrick4", mid = "chartreuse4", midpoint = 2500, limit=c(0,10000))+           #["§§§"] choose one of the commented scale_fill for different colors
	  #scale_fill_viridis_c()+                                                                                                 #["§§§"] choose one of the commented scale_fill for different colors
	  #scale_fill_distiller(palette="green", direction = 1)+                                                                   #["§§§"] choose one of the commented scale_fill for different colors
	  #theme_dark()                 ##["§§§"] choose one of the themes
	  my_theme_facet                ##["§§§"] choose one of the themes	
                    
}

# PRINTING PDF FILE ---------------------------------------------------------------------------------
pdf(file = "AnnWeights_allSim.pdf", width = 16, height = 8 )
ann_plot_list
dev.off()


## testing plot --------------------------------------------------------------------
# ggplot(sim_ann_list[[23]], aes(x=gen, y=bin, fill=count))+
#   geom_tile()+
#   labs(title = titl_name, x="Generations", y=NULL, subtitle = stitl_name)+
#   guides(fill=guide_legend(title="Agents"))+
#   facet_wrap(~node, scale="free", nrow = 2, ncol=4)+
#   scale_x_continuous(breaks = seq(0,1000,50), minor_breaks = NULL)+
#   scale_y_continuous(breaks = seq(-1.6, 1.6, 0.2), minor_breaks = waiver(), limits = c(-1.0,1.0))+
#   #scale_fill_gradient2(low="white",high = "blue",mid = "red", midpoint = 2500, limit=c(0,10000))+
#   scale_fill_gradient2(low="white",high = "firebrick4", mid = "chartreuse4", midpoint = 2500, limit=c(0,10000))+
#   #scale_fill_viridis_c()+
#   #scale_fill_distiller(palette="green", direction = 1)+
#   #theme_dark()
#   my_theme_facet

