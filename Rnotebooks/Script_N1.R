## Script N1 after sourcing the SourceMe file
#This script is the base creator for the analysis work in the following scripts

#calling libraries
library(data.table)
library(tidyverse)

#----------------------------------------------------------------------------------------

#defining the base_element "data"

data_extended <- summary()
data_extended <- data.table(data_extended$agents)
data_extended[, stealing:=1000000-(foraging+handling)]

data <- data_extended[, .(`pop fitness`, foraging, stealing, handling, conflicts)]
data <- as_tibble(data)

#-------------------------------------------------------------------------------------------

#defining the element_base "list_gen" (a list of generation-based data) 
#[THIS TAKES QUITE SOME TIME TO PROCESS]

list_gen <- list()			#creating the empty list
g <- seq(0,500,50)			#vector of the generations to be analysed

#loop cycle for filling list_gen
for (i in 1:length(g)){
  list_gen[i] <- generation(g[i])
}

#-------------------------------------------------------------------------------------
#MyTheme
my_theme <- theme(plot.title = element_text(size=rel(1.5)),
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
