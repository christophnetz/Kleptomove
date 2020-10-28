#### code to make some plots ####

library(tidyr)
library(stringi)
library(glue)
library(ggplot2)

# some helper functions
source("Rnotebooks/helper_functions.R")

# read in the capacity
capacity <- png::readPNG("bin/settings/kernels32.png")[,,1]

max_capacity <- 5L

capacity <- round(capacity * max_capacity, digits = 1)

# simulation type
sim_type <- c("obligate", "facultative", "foragers", "random")

# get the replicates
replicate <- stringr::str_pad(seq_len(2), pad = "0", width = 3)

# growth rates
regrowth <- c(0.001, 0.01, 0.03, 0.05)

# glue data folder names together
data <- crossing(
  sim_type,
  replicate,
  regrowth
)

# get folders
data$folder_path <- glue_data(.x = data,
                              'data/sim_{sim_type}_rep_{replicate}_\\
                                      gro_{regrowth}')

# get data
data$data_summary <- lapply(data$folder_path,
                            get_sim_summary, capacity_matrix = capacity)

# unnest the data
data <- unnest(data,
               cols = "data_summary")

# relevel the layer names for better plotting
data$layer <- forcats::fct_relevel(data$layer,
                                   c("klepts", "foragers", "items",
                                     "klepts_intake", "foragers_intake",
                                     "pc_intake_klepts", 
                                     "pc_intake_forager"))
# relevel the simulation (strategy) type
data$sim_type <- forcats::fct_relevel(data$sim_type, 
                                      "obligate", "facultative", "foragers", "random")

# regroup data
data$layer_type <- dplyr::case_when(
  stringi::stri_detect(data$layer, fixed = "pc_intake") ~ "per_capita_intake",
  stringi::stri_detect(data$layer, fixed = "item") ~ "items",
  stringi::stri_detect(data$layer, fixed = "intake") ~ "intake",
  TRUE ~ "strategy count"
)

# choose layer colours
layer_cols <- tibble(
  layer = c("klepts", "foragers", "items",
            "klepts_intake", "foragers_intake",
            "pc_intake_klepts", 
            "pc_intake_forager"),
  colour = c("indianred", "royalblue", "forestgreen",
             "indianred1", "royalblue1",
             "indianred2", "royalblue2")
)

# merge to data
data_plot <- dplyr::left_join(data,
                              layer_cols)

# split the data by growth rate
data_plot <- split(data_plot, data$layer_type)

#### overall figure ####

plot_list <- lapply(data_plot, function(df) {
  df <- dplyr::filter(df,
                      cap %in% seq(0, 5, 0.2))
  ggplot(df)+
    
    geom_hline(yintercept = 0, col = "grey", lwd = 0.2)+
    geom_vline(xintercept = 0, col = "grey", lwd = 0.2)+
    geom_errorbar(aes(cap,
                    ymin = mean_val - sd_val,
                    ymax = mean_val + sd_val,
                    group = interaction(layer, replicate, regrowth)),
                alpha = 0.5,
                show.legend = F,
                position = position_dodge(width = 0.2),
                col = df$colour)+
    geom_line(aes(cap, mean_val,
                   group = interaction(layer, replicate, regrowth)),
               position = position_dodge(width = 0.2),
               col = df$colour,
              lwd = 0.2)+
    geom_point(aes(cap, mean_val,
                   shape = layer,
                  group = interaction(layer, replicate, regrowth)),
               position = position_dodge(width = 0.2),
               fill = df$colour, 
               colour = "white",
               stroke = 1,
               size = 3,
               show.legend = F)+
    facet_grid(regrowth ~ sim_type, as.table = F,
               scales = "free_y",
               labeller = label_both)+
    scale_shape_manual(values = c(21, 24))+
    coord_cartesian(ylim = c(0, NA))+
    theme_test()+
    theme(legend.position = "top",
          axis.text.y = element_text(size = 6),
          axis.title.y = element_blank())+
    labs(x = "grid cell quality",
         y = "value",
         colour = "value",
         title = sprintf("%s", 
                         unique(df$layer_type)))
})

patchwork::wrap_plots(plot_list[c("strategy count", "intake", 
                                   "items", "per_capita_intake")])

ggsave(filename = "figures/fig_agent_item_distribution.pdf",
       dpi = 300)

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

#### figure item distributions ####
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

