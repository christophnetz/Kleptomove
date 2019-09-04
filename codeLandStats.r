library(data.table)

data = fread("landMoranVals_k.csv")

library(ggplot2)

# plot moran i over gens
ggplot(data)+
  geom_path(aes(x=gen, y=morani))+
  geom_point(aes(x=gen, y=morani), size = 0.2)+
  facet_wrap(~sim, ncol=3)+
  xlim(0,100)+
  geom_hline(yintercept = 0, col=2)
