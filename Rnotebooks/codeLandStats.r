#### code to read in Moran I values ####

#### load libs ####
library(data.table)
library(ggplot2)
library(ggthemes)

#### load data ####
# list land files
landfiles = list.files(pattern = "landMoran")

# read them in
landdata = lapply(landfiles, fread)

# assign identifier to dataframe
for(i in 1:length(landdata)){ landdata[[i]]$version = i}

# rbindlist
landdata = rbindlist(landdata)

# plot moran i over gens with version
# x11()
plotMiGen = ggplot(landdata)+
  geom_path(aes(x=gen, y=morani, 
                lty = factor(version), col = factor(version)))+
  # geom_point(aes(x=gen, y=morani, col = factor(version),
                 # shape = factor(version)))+
  facet_wrap(~sim, ncol=3, scales = "free")+
  scale_color_brewer(palette = "Set1", direction = 1,
                     label = c("klepts", "no klepts"),
                     name = "version")+
  scale_linetype_discrete(#values= c(1,2),
                          label = c("klepts", "no klepts"),
                          name = "version")+
  xlim(0,400)+
  # geom_hline(yintercept = 0, col=1)+
  theme_few()+
  labs(x = "generation (truncated at 400)",
       y = "spatial autocorrelation (Moran's i)")

ggsave(filename = "plotMiGen.pdf", plotMiGen, device = pdf(), width = 20, height = 28, units = "cm"); dev.off()


#### code for independent assessment of land stats ####
# read in landscape
library(raster)

# list images from sim 001 no klepts
files = list.files(path = "bin/settings/", pattern = "sim001",
                   full.names = T)
files = files[1:200]

# read in single file to test
library(png)
d = readPNG(files[3])[,,2]
image(d, col = viridis::inferno(20))

# check NA - should all be zero
apply(d, 2, function(x) sum(is.na(x))) == rep(0,512)

# now convert to raster and get moran's i in a function
funMoran = function(x){
  d = stack(x)
  d = d[[2]]
  mi = Moran(d)
  rm(d); gc()
  return(mi)
}

# map across filenames
library(purrr)
moranlist = map(files[1:10], funMoran)
