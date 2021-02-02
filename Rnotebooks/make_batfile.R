#### code to make simulation batch files ####

library(glue)
library(stringr)
library(tidyr)
library(dplyr)

runs <- str_pad(seq_len(3), width = 3, pad = "0")
strategy <- c(
  "foragers",
  "obligate",
  "facultative",
  "random"
)

# regrowth <- c(0.001, 0.01, 0.03, 0.05, 0.1)

regrowth <- c(0.005, 0.02, 0.04, 0.075, 0.25)

mask <- "{0,0,0}"

# make data
data_param <- crossing(replicate = runs, strategy, regrowth) %>% 
  mutate(outdir = glue('{strategy}_{replicate}'),
         agents.forage = as.numeric(strategy == "foragers"),
         agents.obligate = as.numeric(strategy == "obligate"))



# prepare lines
lines <- glue_data(data_param, 'kleptomove config=../settings/config.ini \\
                   landscape.item_growth={regrowth} \\
                   agents.forage={agents.forage} \\
                   agents.obligate={agents.obligate} \\
                   agents.handling_time=5 outdir=\\
                   ../../data/sim_{strategy}_rep_{replicate}_gro_{regrowth}')

lines[data_param$strategy == "random"] <- 
  glue('{lines[data_param$strategy == "random"]} \\
       agents.input_mask={{0,0,0}')

# write to file
library(readr)
write_lines(lines,
            path = "bin/Release/runs_30_nov_2020_random.bat")
