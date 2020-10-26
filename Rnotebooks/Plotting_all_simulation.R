#Author: Matteo Pederboni [Last modified 13/09/2019]
#This script allows you to plot all the simulations in the source folder (which must be manually inserted below)
#Plots are arranged all togheter in a single "page", but they are all plotted separatedly. 

#getwd()
setwd("C:/Users/matteo/Desktop/OUTPUT_ANALYSIS/21-08-2019_k")

library(tidyverse)
library(data.table)

#list all folders that contain the word "sim"
lfolders <- list.dirs(recursive = FALSE)
folders <- lfolders[ grepl("sim", lfolders) ] #this is to eliminate from the listing every folder not containing "sim"
#folders

summary_list <- list()
#generation_dataframe_list <- list()

for(i in seq_along(folders)){
  source(paste(folders[[i]], "sourceMe.R", sep = "/"))
  
  # SUMMARY LIST ---------------------------------------------------------------
  data_extended <- summary()
  data_extended <- data.table(data_extended$agents)
  data_extended[, stealing:=1000000-(foraging+handling)]
  
  data <- data_extended[, .(`pop fitness`, foraging, stealing, handling, conflicts)]
  data <- as_tibble(data)
  
  parameters <- list(str_sub(config$outdir, start = 4), config$agents.handling_time, config$landscape.item_growth)
  
  summary_list[[i]] <- list(data, parameters)
  
  # GENERATION DATAFRAME LIST -----------------------------------------------------
  #[da fare ancora perchè al momento non ci serve]
}

#MyTheme
my_theme <- theme(#plot.title = element_text(size=rel(1.5)),
                  panel.grid.major = element_line(color = "grey90"), 
                  panel.background = element_rect(fill = "white", colour = NULL),
                  axis.line = element_line(size = 1, colour = "black"),
                  strip.background = element_rect(colour = "black", fill = "white"),
                  strip.text = element_text(colour = "black", face = "bold"))

my_theme_facet <- theme(plot.title = element_text(size=rel(1.5)),
                        panel.grid.major = element_line(color = "grey90"), 
                        panel.background = element_rect(fill = "white", colour = "grey70"),
                        axis.line = element_line(size = 1, colour = "black"),
                        strip.background = element_rect(colour = "black", fill = "white"),
                        strip.text = element_text(colour = "black", face = "bold"))


simplot_list <- list()
for (i in seq_along(summary_list)){
  data <- summary_list[[i]][[1]]
  sim <- summary_list[[i]][[2]][[1]]
  par_hand <- summary_list[[i]][[2]][[2]]
  par_igrowth <- summary_list[[i]][[2]][[3]]
  
  title <- paste("ActivityRatio", sim, sep=" ")
  subtitle <- paste("handling", par_hand, "/growthrate", par_igrowth, sep = " ")
  #Plot different actions (proportions per generations)
  act <- ggplot(data)+
    geom_line(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="deepskyblue4")+
    geom_line(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="chartreuse4")+
    geom_line(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="firebrick3")+
    labs(x="Generations", y="prop activity", title = title, subtitle = subtitle)+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = waiver(), limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,1,0.2), minor_breaks = waiver())+
    my_theme


  #act

  # act_smooth <- ggplot(data)+
  #   geom_smooth(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="blue")+
  #   geom_smooth(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="green")+
  #   geom_smooth(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="red")+
  #   labs(x="Generations", y="prop activity", title = "Proportion of Individuals grouped for activity")+
  #   scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
  #   scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
  #   my_theme

  #act_smooth

  #---------------------------------------------------------------
  #Plot average fitness
  avg_fit <- ggplot(data)+
    geom_line(aes( y= `pop fitness`, x= 1:dim(data)[1] ))+
    labs(x="Generations", y="average fitness", title = "Population fitness", subtitle = subtitle)+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,20,1), minor_breaks = NULL, limits = c(0, NA))+
    my_theme

  avg_fit

  #----------------------------------------------------------------
  #Stealing efficiency trough time (generations)
  stl_eff <- ggplot(data)+
    geom_line(aes(y=conflicts/stealing,x=1:dim(data)[1]), na.rm=TRUE)+
    labs(x="Generations", y="stealing success/stealing attempt", title = "Effectiveness of stealing", subtitle = subtitle)+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
    my_theme

  #stl_eff

  #filling simplot_lsit
  simplot_list[[i]] <- list(act, avg_fit, stl_eff)
}

# Creating list of plots -------------------------------------------------------
action_list_plot <- list()
for(i in seq_along(simplot_list)){action_list_plot[[i]] <- simplot_list[[i]][[1]]}

average_fitness_list_plot <- list()
for(i in seq_along(simplot_list)){average_fitness_list_plot[[i]] <- simplot_list[[i]][[2]]}

stealing_effic_list_plot <- list()
for(i in seq_along(simplot_list)){stealing_effic_list_plot[[i]] <- simplot_list[[i]][[3]]}

# Exporting PLOTS ------------------------------------------------------------------
library(ggpubr)
arr_act <- ggarrange(plotlist = action_list_plot, ncol = 3, nrow = 9)
ggexport(arr_act, filename = "activity_ratio_graphs.pdf", width = 16, height = 22)

arr_avg_fit <- ggarrange(plotlist = average_fitness_list_plot, ncol = 3, nrow = 9)
ggexport(arr_avg_fit, filename = "population_fitness_graphs.pdf", width = 16, height = 22)

arr_stl_eff <- ggarrange(plotlist = stealing_effic_list_plot, ncol = 3, nrow = 9)
ggexport(arr_stl_eff, filename = "effectiveness_stealing_graphs.pdf", width = 16, height = 22)
