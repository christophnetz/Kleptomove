#### code to make simulation batch files ####

library(glue)
library(stringr)
library(dplyr)

runs <- str_pad(seq_len(20), width = 3, pad = "0")
strategies <- c("strat_forg", "strat_fixd", "strat_flex")

# make data
data_param <- crossing(replicate = runs, strategies) %>% 
  mutate(outdir = glue('{strategies}_{replicate}'),
         agents.forage = if_else(strategies == "strat_forg", 1, 0),
         agents.obligate = if_else(strategies == "strat_fixd", 1, 0))

data_param <- filter(data_param, strategies == "strat_fixd")
# prepare lines
lines <- glue_data(data_param, 'kleptomove config=../settings/config.ini \\
                   G=1000 landscape.item_growth=0.010 \\
                   agents.forage={agents.forage} \\
                   agents.obligate={agents.obligate} \\
                   agents.handling_time=5 outdir=sim{outdir}')

# write to file
library(readr)
write_lines(lines,
            path = "bin/Release/runs_18_aug_fixed_2020.bat")
