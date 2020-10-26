#Author: Matteo Pederboni [Last modified: 13/09/2019]
#THIS SCRIPT DOES NOT WORKS THROUGH FOLDERS (YET). YOU HAVE TO SOURCE THE SIMULATION YOU WANT THE PLOT FIRST

setwd("C:/Users/matteo/Desktop/OUTPUT_ANALYSIS/modelOutput_klepts_20_09_19/images")  #PLEASE, BE AWARE OF THE FOLDER IN WHICH YOU ARE PLOTTING
origin <- getwd()

#calling libraries
library(data.table)
library(tidyverse)
library(ggpubr)

#---------------------------------------------------------
list_gen <- list()			#creating the empty list
g <- seq(0,950,5)			#THIS IS VERY IMPORTANT! WHATCH OUT HOW BIG YOU WANT THIS

#loop cycle for filling list_gen
for (i in 1:length(g)){
  list_gen[i] <- generation(g[i])
}

#------------------------------------------------------
#Creating an empty list
y <- list()

#Filling the list
for (i in 1:length(list_gen)){
  y[[i]] <- tibble(
    id = c(1:length(list_gen[[i]]$fit)),
    ft = as.vector(list_gen[[i]]$foa),
    ht = as.vector(list_gen[[i]]$han),
    st = as.vector(100-(ft+ht)),
    energy = as.vector(list_gen[[i]]$fit)
  )
}

#-------------------------------------------------------------

#
y_compressed <- map2_df(y,g, function(i,j){mutate(i,gen=j)})

library(ggtern)

# #this is for creating a FACET GRAPH
# ggtern(b, aes(ft,ht,st))+
#   geom_point(aes(color=energy), size=2, alpha=0.3)+
#   theme_showarrows()+
#   theme_ticklength(major = unit(2,"mm"))+
#   xlab("Foraging") + ylab("Handling") +  zlab("Stealing")+
#   scale_color_viridis_c()+
#   facet_wrap(~gen)
# 
# #this creates a single graph. it refers to "a" (a single generation-time)
# a <- y[[30]]
# image <- ggtern(a, aes(ft,ht,st))+
#   geom_point(aes(color=energy), size=2, alpha=0.3)+
#   theme_showarrows()+
#   theme_ticklength(major = unit(2,"mm"))+
#   xlab("Foraging") + ylab("Handling") +  zlab("Stealing")+
#   theme(plot.title = element_text(size=40))+
#   scale_color_viridis_c()
#   #facet_wrap(~energy) #you want this to be faceted by energy?
# 
# image
# 
#ggexport(image, filename ="test.png", width = 1024, height = 1024)
# 

# For Making gifs (or multiple single plots)---------------------------
path_img <- paste(origin, str_sub(config$outdir, start = 4), sep="/")
setwd(path_img)

my_tern_theme <- theme(plot.title = element_text(size = 40),
                       tern.axis.title = element_text(size=25), 
                       tern.axis.arrow.text = element_text(size = 15),
                       tern.axis.arrow = element_line(size = 2),
                       legend.title = element_text(size = 15))

stitl <- paste("handling", as.character(config$agents.handling_time), "/growthrate", as.character(config$landscape.item_growth), sep = " ")

for (i in 1:length(y)){
  image <- ggtern(y[[i]], aes(ft,ht,st))+
    geom_point(aes(color=energy), size=2, alpha=0.3)+
    labs(title = as.character(g[[i]]), subtitle = stitl)+
    xlab("Foraging") + ylab("Handling") +  zlab("Stealing")+
    scale_color_viridis_c(limits=c(0,max(y_compressed$energy)))+
    my_tern_theme+
    guides(color = guide_colorbar(barwidth = 2, barheight = 12))+
    theme_showarrows()+
    theme_ticklength(major = unit(2,"mm"))

  simulation <- str_sub(config$outdir, start = 4)
  gx <-  g[[i]]
  imag_name <- paste("ImageTern", simulation, gx, ".png", sep="_")
  
  ggexport(image, filename = imag_name, width = 1024, height = 1024)
}
