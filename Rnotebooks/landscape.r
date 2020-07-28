#### code to make some plots ####

library(tseries)
library(data.table)
library(purrr)
library(magrittr)
library(glue)
library(ggplot2)

# where is the output
data_folder <- "data"

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
type <- c("facultative", "foragers", "obligate")

# get the replicates
replicates <- c("rep1","rep2","rep3")

# get layers
layers <- c("items", "foragers", "klepts", "klepts_intake", "foragers_intake")

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
  # # summarise
  # reps <- reps[, .(mean = mean(value),
  #                  sd = sd(value)),
  #              by = cell_capacity]
})

# assign rep number and layer name
data <- map_depth(data, 2, function(l) {
  map2(l, names(l), function(z1, z2) {
    z1[, repl := z2]
  })
}) %>% 
  map_depth(2, data.table::rbindlist)

# assign layer and sim type names
data <- imap(data, function(x, v) {
  map2(x, names(x), function(z, w) {
    z[, `:=` (type = v,
              layer = w)]
  })
})

# get final data for plotting
data <- map(data, rbindlist) %>% 
  rbindlist()

data[, layer := forcats::fct_relevel(layer,
                                     c("klepts", "foragers", "items",
                                       "klepts_intake", "foragers_intake"))]
data[, type := forcats::fct_relevel(type, 
                                    "foragers", "obligate", "facultative")]

data2 <- dcast(data, 
               type + repl + cell_capacity  ~ layer, 
               fun.aggregate = mean, value.var = "value")

data2[, `:=`(pc_int_klept = klepts_intake / klepts,
             pc_int_forager = foragers_intake / foragers)]

# get per capita intake
data2[, `:=`(pc_int_klept = ifelse(klepts_intake == 0, 0, pc_int_klept),
             pc_int_forager = ifelse(foragers_intake == 0, 0, pc_int_forager))]

# melt for facetting with items
data2 <- melt(data2, id.vars = c("type", "repl", "cell_capacity"))

# separate by intake
data2[, type2 := dplyr::case_when(
  stringr::str_detect(variable, "intake") ~ "intake",
  stringr::str_detect(variable, "item") ~ "items",
  stringr::str_detect(variable, "pc") ~ "per_capita_intake",
  TRUE ~ "strategy count"
)]

#### make some plot ####
ggplot(data2)+
  geom_path(aes(cell_capacity,
                value,
                group = repl,
                col = variable))+
  geom_point(aes(cell_capacity,
                value,
                group = repl,
                col = variable))+
  scale_colour_manual(values = pals::tol.rainbow(7))+
  theme_minimal()+
  facet_grid(type2~type,
             scales = "free_y")

ggsave(filename = "figures/fig_agent_item_distribution.png",
       dpi = 300, width = 8, height = 8)


#### per capita intake ####

# get per capita intake
data3 <- data2[str_detect(layer, "items", negate = T),] %>% 
  dcast(type + cell_capacity + repl ~ layer, 
        value.var = "value", 
        fun.aggregate = sum)

data3[, `:=`(inpc_forager = foragers_intake / foragers,
             inpc_klept = klepts_intake / klepts)]

data3 <- melt(data3,
              id.vars = setdiff(colnames(data3), c("inpc_forager",
                                                   "inpc_klept")))

ggplot(data3)+
  geom_boxplot(aes(factor(cell_capacity),
                   value,
                   fill = variable),
               notch = T)+
  theme(legend.position = "top")+
  scale_fill_brewer(palette = "Pastel1", direction = -1)+
  facet_grid(~type,
             scales = "free_y")+
  # scale_y_sqrt()+
  coord_cartesian(ylim = c(0, 1))

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

