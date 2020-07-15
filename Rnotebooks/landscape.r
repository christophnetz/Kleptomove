#### code to make some plots ####

library(tseries)
library(data.table)
library(purrr)
library(magrittr)
library(glue)
library(ggplot2)

# where is the output
data_folder <- "bin/Release/landscapes"

# read in the capacity
capacity <- read.matrix(glue('{data_folder}/capacity_bitmap_v4_channel2.txt'), 
                        sep="\t")
max_capacity <- 5L

# convert the capacity matrix
capacity <- data.table(capacity)
setnames(capacity, as.character(seq_len(512)))
capacity[, y := as.character(seq_len(512))]
capacity <- data.table::melt.data.table(capacity,
                                        id.vars = "y",
                                        variable.name = "x",
                                        value.name = "cell_capacity")
capacity[, x := as.character(x)]
capacity[, cell_capacity := floor(cell_capacity * max_capacity)]

# get generations
which_gen <- seq(991, 999)

# get simulation type
type <- c("facultative", "obligate")

# get the replicates
replicates <- c("rep1","rep2","rep3")

# get layers
layers <- c("items", "foragers", "klepts")

# glue data file names together
data_files <- tidyr::crossing(
  which_gen,
  type,
  replicates,
  layers
)
data_files$filepath <- glue_data(.x = data_files,
                        '{data_folder}/{which_gen}{type}_\\
                                  {replicates}{layers}.txt')

# split by layer and replicate
data <- data_files %>% 
  split(data_files$type) %>% 
  map(function(l) {
    l %>% 
      split(l$layers) %>% 
      map(function(l) {
        l %>% 
          split(l$replicates) %>% 
          map(function(r) {
            r$filepath
          })
      })
  })
  

# read in data and get mean of layers per replicate
data <- map_depth(data, 4, read.matrix) %>% 
  map_depth(3, function(gens) {
    reduce(gens, .f = `+`) / length(gens)
  })

# convert to dataframe for capacity wise mean
data <- map_depth(data, 3, function(reps) {
  # convert with colnames
  data_reps <- data.table::data.table(reps)
  data.table::setnames(data_reps, as.character(seq_len(512)))
  
  # assign y coord
  data_reps[, y := as.character(seq_len(512))]
  
  # now melt
  data_reps <- data.table::melt.data.table(data_reps,
                                           id.vars = "y",
                                           variable.name = "x")
  
  # fix x coord
  data_reps[, x := as.character(x)]
  
  return(data_reps)
})

# get capacity wise mean
data <- map_depth(data, 3, function(reps) {
  # merge capacity
  reps <- merge(reps, capacity)
  # summarise
  reps <- reps[, .(mean = mean(value),
                   sd = sd(value)),
               by = cell_capacity]
})

# assign rep number and layer name
data <- map_depth(data, 2, function(l) {
  map2(l, names(l), function(x, y) {
    x[, repl := y]
  })
}) %>% 
  map_depth(2, data.table::rbindlist)

# assign layer and sim type names
data <- imap(data, function(x, y) {
  map2(x, names(x), function(z, w) {
    z[, `:=` (type = y,
              layer = w)]
  })
})

# get final data for plotting
data <- map(data, rbindlist) %>% 
  rbindlist()

data[, layer := forcats::fct_relevel(layer,
                                     c("klepts", "foragers", "items"))]

#### make some plot ####
ggplot(data) +
  geom_ribbon(aes(cell_capacity, 
                  ymin = mean + 1 - sd,
                  ymax = mean + 1 + sd,
                  group = interaction(repl, layer),
                  fill = layer),
              alpha = 0.2)+
  geom_line(aes(cell_capacity, mean + 1,
                group = interaction(repl, layer),
                col = layer),
            size = 1, alpha = 0.5) +
  scale_colour_brewer(palette = "Set1")+
  scale_fill_brewer(palette = "Set1")+
  theme_grey()+
  theme(legend.position = "top")+
  facet_grid(~ type)+
  coord_cartesian(expand = F)+
  scale_y_log10(breaks = c(1, 10, 100),
                labels = c(0, 10, 100))+
  labs(x = "cell capacity (items)",
       y = "mean value")

ggsave(filename = "figures/fig_agent_item_distribution.png",
       dpi = 300, width = 6, height = 4)

# 
# ggplot(data=ourdata, aes(x=factor(samples, level = unique(ourdata$samples)), y=f_id, fill=items)) + 
#   geom_raster() +
#   scale_fill_viridis_c(
#                        limits=c(0,10))+ scale_y_discrete(breaks=unique(x2$f_id)[seq(1,length(unique(x2$f_id)),by=32)]) +
#   scale_x_discrete(breaks=unique(x2$samples)[seq(1,length(unique(x2$samples)),by=32)])
# 
# 
# 
# 
# 
# ourdata <- ourdata[order(ourdata$capacity),]
# ourdata$rowid <- 1:length(ourdata$f_id)
# 
# plot1 <- ggplot(ourdata, aes(x=rowid, y=items))+
#   geom_point(colour="green")+
#   geom_point(aes(x=rowid, y=foragers), colour="blue")+
#   geom_point(aes(x=rowid, y=klepts), colour="red")
# 
# ourdata <- ourdata[order(ourdata$foragers),]
# ourdata$rowid2 <- 1:length(ourdata$f_id)
# 
# plot2 <- ggplot(ourdata, aes(x=rowid2, y=foragers))+
#   geom_point(colour="blue")+
#   geom_point(aes(x=rowid, y=klepts), colour="red", size=0.1)+
#   geom_point(aes(x=rowid, y=items), colour="green", size=0.1)
#   
# ggplot(ourdata, aes(x=capacity, y=foragers))+
#   geom_point(colour="blue")+
#   geom_point(aes(x=capacity, y=klepts), colour="red", size=0.1)


####### Reaction norms

library(plotly)
library(magrittr)

G <- generation(4000)
G$agents$ann

mcr <- function(x, n, drop = FALSE) { #'most common row' 
  xx <- do.call("paste", c(data.frame(x), sep = "\r")) 
  tx <- table(xx)
  sx = sort(tx,decreasing=TRUE)
  mx <- names(sx)[n[1]] 
  x[match(mx, xx), , drop = drop] 
}
mcann <- mcr(G$agents$ann, 3)

eq = function(i1, i2, i3){as.numeric((mcann[1,5] + i1*mcann[1,6] + i2*mcann[1,7] + i3*mcann[1,8]) >= 0)}
eq2 = function(i1, i2, i3){(mcann[1,5] + i1*mcann[1,6] + i2*mcann[1,7] + i3*mcann[1,8])}


x <- y <- z <- seq(0, 1, length.out = 50)
xyz <- mesh(x, y, z)
x <- as.vector(xyz$x)
y <- as.vector(xyz$y)
z <- as.vector(xyz$z)


fig <- plot_ly(
  type='isosurface',
  x = x,
  y = y,
  z = z,
  value = eq(x,y,z),
  isomin=0.5,
  isomax=2
)

fig <- fig %>% layout(
  title = config$outdir,
  scene = list(
    xaxis = list(title = "nonhandlers"),
    yaxis = list(title = "handlers"),
    zaxis = list(title = "items")
  ))


fig

