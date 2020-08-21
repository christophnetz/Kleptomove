#### code to make some plots ####

library(data.table)
library(purrr)
library(magrittr)
library(glue)
library(ggplot2)

# where is the output
data_folder <- "data"

# read in the capacity
capacity <- png::readPNG("bin/settings/test_small.png")[,,3]
max_capacity <- 5L

# convert the capacity matrix
capacity <- data.table(capacity)
setnames(capacity, as.character(seq_len(128)))
capacity[, y := as.character(seq_len(128))]
capacity <- data.table::melt.data.table(capacity,
                                        id.vars = "y",
                                        variable.name = "x",
                                        value.name = "cell_capacity")
capacity[, x := as.character(x)]
capacity[, cell_capacity := floor(cell_capacity * max_capacity)]

# get generations
which_gen <- seq(991, 999)

# get simulation type
type <- c("fixd", "forg", "flex")

# get the replicates
replicates <- stringr::str_pad(seq_len(10), pad = "0", width=3)

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
                        '{data_folder}/simstrat_{type}_\\
                                  {replicates}/{which_gen}{layers}.txt')

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
  data.table::setnames(data_reps, as.character(seq_len(128)))
  
  # assign y coord
  data_reps[, y := as.character(seq_len(128))]
  
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
                                    "forg", "fixd", "flex")]

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

#### figure agent strategy distributions ####
fig_strat_distr <- data2[type2 == "strategy count", ] %>%
  split(by = c("type")) %>% 
  map(function(d) {
    ggplot(d)+
      geom_line(aes(cell_capacity,
                    value,
                    group = interaction(variable, type, repl),
                    col = variable))+
      geom_point(aes(cell_capacity,
                     value,
                     group = repl,
                     col = variable),
                 shape = 16, size = 2)+
      scale_colour_manual(values = rev(pals::coolwarm(2)))+
      theme_bw()+
      theme(panel.grid.minor = element_blank(),
            legend.position = "top",
            plot.background = element_rect(fill = NA,
                                           colour = "grey", 
                                           linetype = 3))+
      facet_grid(type2~type,
                 scales = "free_y")+
      # xlim(0, 4)+
      ylim(0, 11)+
      labs(x = "cell capacity (# items)",
           y = "# agents (median)")
  })

patchwork::wrap_plots(fig_strat_distr)+
  patchwork::plot_annotation(tag_levels = "A")+
  theme(plot.tag = element_text(face = "bold"))

ggsave(filename = "figures/fig_agent_strat_distribution.png",
       dpi = 300, width = 9, height = 4)

♣#### figure item distributions ####
fig_item_distr <- data2[type2 == "items", ] %>%
  split(by = c("type")) %>% 
  map(function(d) {
    ggplot(d)+
      geom_line(aes(cell_capacity,
                    value / (100 * 10),
                    group = interaction(variable, type, repl),
                    col = variable))+
      geom_point(aes(cell_capacity,
                     value / (100 * 10),
                     group = repl,
                     col = variable),
                 shape = 16, size = 2)+
      scale_colour_manual(values = "forest green")+
      theme_bw()+
      theme(panel.grid.minor = element_blank(),
            legend.position = "top",
            plot.background = element_rect(fill = NA,
                                           colour = "grey", 
                                           linetype = 3))+
      facet_grid(type2~type,
                 scales = "free_y")+
      # xlim(0, 4)+
      ylim(0, 0.4)+
      labs(x = "cell capacity (# items)",
           y = "# items (median / 100 t / 10 g)")
  })

patchwork::wrap_plots(fig_item_distr)+
  patchwork::plot_annotation(tag_levels = "A")+
  theme(plot.tag = element_text(face = "bold"))

ggsave(filename = "figures/fig_item_density_distribution.png",
       dpi = 300, width = 9, height = 4)

#### figure per capita intake ####
fig_pc_intake <- data2[type2 %in% c("per_capita_intake"), ] %>%
  split(by = c("type")) %>%
  map(function(d) {
    ggplot(d)+
      geom_line(aes(cell_capacity,
                    value,
                    group = interaction(variable, type, repl),
                    col = variable))+
      geom_point(aes(cell_capacity,
                     value,
                     group = repl,
                     col = variable),
                 shape = 16, size = 2)+
      scale_colour_manual(values = rev(pals::kovesi.diverging_cwm_80_100_c22(2)))+
      theme_bw()+
      theme(panel.grid.minor = element_blank(),
            legend.position = "top",
            plot.background = element_rect(fill = NA,
                                           colour = "grey", 
                                           linetype = 3))+
      facet_grid(type2~type,
                 scales = "free_y")+
      xlim(0, 4)+
      ylim(0, 0.2)+
      labs(x = "cell capacity (# items)",
           y = "# items (median)")
  })

patchwork::wrap_plots(fig_pc_intake)+
  patchwork::plot_annotation(tag_levels = "A")+
  theme(plot.tag = element_text(face = "bold"))

ggsave(filename = "figures/fig_pc_intake.png",
       dpi = 300, width = 9, height = 4)

#### plot pc intake vs agents ####

# read data
data <- fread("data/data_full.csv")

d2 <- copy(data)
d2 <- dcast(data, type + repl + cell_capacity + x + y ~ layer,
            value.var = "value")

# count total agents
d2[, `:=`(total_agents = foragers + klepts,
          total_intake = foragers_intake + klepts_intake)]

# get per capita intake
d2[, `:=`(pc_intake_total = total_intake / total_agents,
          pc_intake_klept = klepts_intake / total_agents,
          pc_intake_forager = foragers_intake / total_agents)]

# plot per capita intake over total agents
d3 <- copy(d2)
d3 <- d3[, .(pc_intake_total = mean(pc_intake_total),
             pc_intake_forager = mean(pc_intake_forager),
             pc_intake_klept = mean(pc_intake_klept)),
   by = .(type, repl, total_agents, cell_capacity)]

# plot
ggplot(d3)+
  # geom_point(aes(y=pc_intake_total, x=total_agents,
  #                col = cell_capacity), 
  #            size = 0.1, shape = 4)+
 
  geom_point(aes(y=pc_intake_forager, x = total_agents,
                 col= cell_capacity), 
             col = "blue", alpha = 0.1,
             size = 0.05, shape = 1)+
  geom_point(aes(y=pc_intake_klept, x = total_agents,
                 col = cell_capacity),
             # col = "red", 
             size = 0.05, shape = 2
             #alpha = 0.1
             )+
  
  facet_grid(type~ repl)+
  scale_colour_distiller(palette = "YlOrRd",
                         direction = 1)+
  theme_bw()+
  theme(legend.position = "top")+
  xlim(0, 20)+
  labs(y = "per capita intake by strategy (mean)",
       x = "# total agents on grid cell (both strategies)",
       fill = "cell carrying capacity")

ggsave(filename = "figures/fig_pc_intake_vs_agents_hi_klept.png",
       dpi = 300)

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

