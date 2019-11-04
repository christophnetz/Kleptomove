#Author: Matteo Pederboni [Last modified 13/09/2019]
#This script allows you to plot all the simulations in the source folder (which must be manually inserted below)
#Plots are in a grid and they are "facetted" so that it is possible to search for COMBINATION OF PARAMETERS. 

#getwd()
setwd("C:/Users/matteo/Desktop/OUTPUT_ANALYSIS/modelOutput_klepts_20_09_19")

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
  data <- as_tibble(data) %>% 
    mutate(simulation = str_sub(config$outdir, start = 4),
           par_hand = config$agents.handling_time, 
           par_igrowth = config$landscape.item_growth,
           gen = c(1:1000))
           
  summary_list[[i]] <- data
  # GENERATION DATAFRAME LIST -----------------------------------------------------
}

dataframe_complete <- bind_rows(summary_list) %>% 
  mutate(repl=rep(rep(c(1:3),each=1000),each=9))

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

# dataframe_complete <- dataframe_complete%>% 
# mutate(gen=rep(c(1:1000),27))

# Plot ActivityRatio ---------------------------------------------------------------------------
act <- ggplot(dataframe_complete)+
    geom_line(aes( y= foraging/1000000, x=gen, group=simulation), col="deepskyblue4")+
    geom_line(aes( y= handling/1000000, x=gen, group=simulation), col="chartreuse4")+
    geom_line(aes( y= stealing/1000000, x=gen, group=simulation), col="firebrick3")+
    labs(x="Generations", y="Activity Frequency",title = "ActivityRatio")+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = waiver(), limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = NULL)+
    facet_grid(par_hand ~ par_igrowth, labeller = label_both)+
    my_theme_facet
  
#act

act2 <- ggplot(dataframe_complete)+
  geom_line(aes( y= foraging/1000000, x=gen), col="deepskyblue4")+
  geom_line(aes( y= handling/1000000, x=gen), col="chartreuse4")+
  geom_line(aes( y= stealing/1000000, x=gen), col="firebrick3")+
  labs(x="Generations", y="Activity Frequency",title = "ActivityRatio")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = waiver(), limits = c(0,1000))+
  scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = NULL)+
  facet_grid(par_hand + repl ~ par_igrowth, labeller = label_both)+
  my_theme_facet

#act2
 
# Plot average fitness  ---------------------------------------------------------------
avg_fit <- ggplot(dataframe_complete)+
    geom_line(aes( y= `pop fitness`, x= gen , group=simulation, colour=factor(repl)))+
    labs(x="Generations", y="Average population fitness", title = "Population fitness", subtitle = "visualization of the average value over time")+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,20,1), minor_breaks = NULL, limits = c(0, NA))+
    facet_grid(par_hand ~ par_igrowth, scales = "free_y", labeller = label_both)+
    scale_colour_grey()+
    my_theme_facet

#avg_fit

# Plot Stealing efficiency trough time (generations) ----------------------------------------------
stl_eff <- ggplot(dataframe_complete)+
    geom_line(aes(y=conflicts/stealing,x=gen, group=simulation, colour=factor(repl)), na.rm=TRUE)+
    labs(x="Generations", y="stealing success/stealing attempt", title = "Effectiveness of Stealing")+
    scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
    scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
    facet_grid(par_hand ~ par_igrowth, scales = "free_y", labeller = label_both)+
    scale_colour_grey()+
    my_theme

#stl_eff

# Plotting percentage of behaviors ------------------------------------+
perc_behav <-  ggplot(dataframe_complete)+
  geom_line(aes( y= foraging/(foraging+stealing), x=gen, group=simulation), col="navy")+
  geom_line(aes( y= stealing/(foraging+stealing), x=gen, group=simulation), col="firebrick3")+
  labs(x="Generations", y="Behavior")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = waiver(), limits = c(0,1000))+
  scale_y_continuous(labels = scales::percent)+
  facet_grid(par_hand + repl ~ par_igrowth)+
  my_theme_facet

# EXPORTING PLOTS -----------------------------------------------------------------------------
library(ggpubr)
ggexport(act, filename = "ActivityRatio_replicas_overlapping.pdf", width = 11.5, height = 8)
ggexport(act2, filename = "ActivityRatio_expanded.pdf", width = 16, height = 22)
ggexport(avg_fit, filename = "fitness_replicas_overlapping.pdf", width = 11.5, height = 8)
ggexport(stl_eff, filename = "stealEff_replicas_overlapping.pdf", width = 11.5, height = 8)
ggexport(perc_behav, filename="Perc_behaviors.pdf", width = 16, height = 22)