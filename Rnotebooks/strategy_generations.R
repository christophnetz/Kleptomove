#### plot strategies over generations ####

library(data.table)
library(ggplot2)

data <- fread("data/data_strategy_generations.csv")

# relevel factors
data$strategy <- factor(data$strategy,
                        levels = c("forg", "fixd", "flex"),
                        labels = c("foraging only",
                                   "fixed strategy",
                                   "flexible strategies"))

ggplot(data[strategy == "fixed strategy",])+
  geom_line(aes(gen, value, 
                col = variable,
                group = interaction(variable, strategy, replicate)))+
  facet_grid(~strategy)+
  scale_colour_manual(values = c("blue", "forestgreen", "red"))+
  theme_bw()+
  theme(legend.position = "top")+
  labs(x = "generation",
       y = "p(strategy) in population")

ggsave(filename = "figures/fig_strategy_generations.png",
       dpi = 300, width = 8, height = 4)
