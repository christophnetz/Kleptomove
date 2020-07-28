#### code to extract data from multiple sims ####

#### load libs
library(data.table)
library(purrr)
library(stringr)

#### list data directories
# get full names of data containing directories
data_dirs <- list.dirs(path = "bin/output/modelOutput_klepts_20_09_19",
                       full.names = TRUE,
                       recursive = FALSE)

# exclude nonsense directories
data_dirs <- data_dirs[stringr::str_detect(data_dirs, "sim")]

# write a function to run sourceMe.r and get summary data
funcGetData <- function(filepath){
  
  # check for character
  assertthat::assert_that("character" %in% class(filepath), 
                          msg = "filepath is not a string")
  
  # get filepath of the function sourceME.r
  toSource <- list.files(path = filepath,
                         pattern = "sourceMe.R",
                         full.names = TRUE)
  
  # source it
  source(toSource)
  # get summary data
  temp_summary <- summary() # who decided to name this after a common R func?
  # get agent data
  temp_summary <- data.table(temp_summary$agents)
  
  # get sim identifier
  sim <- as.numeric(stringr::str_split(filepath, "sim")[[1]][2])
  
  # add gen and sim identifier
  temp_summary[, `:=`(gen = 1:nrow(temp_summary),
                      sim = sim)]
  # remove spaces in names
  setnames(x = temp_summary, 
           old = names(temp_summary),
           new = str_replace(string = names(temp_summary),
                             pattern = " ",
                             replacement = "_"))
  # return df
  return(temp_summary)
}

# run on the filepaths in data_dirs to return dfs
data_summary <- purrr::map_df(data_dirs, funcGetData)

# melt by gen, sim, variable
data_melt <- melt(data_summary,
                  id.vars = c("gen", "sim"))

#### plot values over generations in each sim
# load plot libs
library(ggplot)
library(ggthemes)
library(scico)

# plot
ggplot(data_melt)+
  geom_line(aes(x = gen, y = value, col = variable))+
  ggthemes::theme_clean()+
  scale_colour_scico_d()+
  scale_x_continuous(breaks = seq(0,1e3, 5e2))+
  facet_grid(variable ~ sim, scales = "free")

# ends here