# Cinema generated script, do not edit.

config = list(
  Gburnin=0,
  G=1000,
  T=100,
  Gfix=1000,
  Tfix=100,
  outdir="obligate_rep3",
  omp_threads=1,

  agents.N=10000,
  agents.L=5,
  agents.ann="SimpleAnn",
  agents.obligate=1,
  agents.sprout_radius=1,
  agents.flee_radius=5,
  agents.handling_time=5,
  agents.mutation_prob=0.001,
  agents.mutation_step=0.01,
  agents.mutation_knockout=0,
  agents.noise_sigma=0.1,
  agents.cmplx_penalty=0,
  agents.input_layers=c(8,2,3),
  agents.input_mask=c(1,1,1),

  landscape.max_item_cap=5,
  landscape.item_growth=0.01,
  landscape.detection_rate=0.2,
  landscape.capacity.image="bitmap_v4.png",
  landscape.capacity.channel=2,

# Metadata
  agents.ann.weights = 8
)


# auxiliary function
import.raw <- function(filename, type, size) {
  finfo <- file.info(filename)
  if (is.na(finfo$size)) stop(paste(fname, "doesn't exist"))
  con <- file(filename, "rb", raw = TRUE)
  n <- finfo$size / size
  buf <- readBin(con, type, n, size=size)
  close(con)
  buf
}

# auxiliary function
import.generation <- function(G, what, stderr) {
  if (!(what=="pred" || what=="agents")) stop("argument what shall be 'agents' or 'pred'")
  extractor <- paste0(config$dir, '/depends/extract.exe')
  Args <- paste0('G="', toString(G), '" ' , "dir=", config$dir, " what=", what)
  system2(extractor, args=Args, stderr=stderr)
  ann <- matrix(import.raw(paste0(config$dir, "/tmp/", what, "_ann.tmp"), numeric(), 8),
                ncol=config[[paste0(what, ".ann.weights")]], byrow=T)
  fit <- import.raw(paste0(config$dir, "/tmp/", what, "_fit.tmp"), numeric(), 8)
  anc <- import.raw(paste0(config$dir, "/tmp/", what, "_anc.tmp"), integer(), 4)
  foa <- import.raw(paste0(config$dir, "/tmp/", what, "_foa.tmp"), numeric(), 8)
  han <- import.raw(paste0(config$dir, "/tmp/", what, "_han.tmp"), numeric(), 8)
  system2(extractor, paste0("dir=", config$dir, " --cleanup"))
  list(ann=ann, fit=fit, anc=anc, foa=foa, han=han)
}

# extract generation
# params:
#   G    : generation
generation <- function(G, stderr=F) {
  agents = import.generation(G, "agents", stderr)
  list(agents=agents)
}

# load input estimates
input <- function() {
  agents = matrix(import.raw(paste0(config$dir, '/agents_input.bin'), numeric(), 8), ncol=15, byrow=T)
#  pred = matrix(import.raw(paste0(config$dir, '/pred_input.bin'), numeric(), 8), ncol=15, byrow=T)
  cn <- c('min0', 'max0', 'mean0', 'std0', 'mad0', 
          'min1', 'max1', 'mean1', 'std1', 'mad1', 
          'min2', 'max2', 'mean2', 'std2', 'mad2') 
  colnames(agents) <- cn
#  colnames(pred) <- cn
  list(agents=agents) #CN: pred excluded , pred=pred
}
  
# load summary
summary <- function() {
  agents = matrix(import.raw(paste0(config$dir, '/agents_summary.bin'), numeric(), 8), ncol=8, byrow=T)
  cn <- c('pop fitness', 'repro fitness', 'repro ind', 'repro clades', 'complexity', 'foraging', 'handling', 'conflicts')
  colnames(agents) <- cn
#  colnames(pred) <- cn
  list(agents=agents) #CN: pred excluded, pred=pred
}
  
config$dir = getSrcDirectory(generation)[1]
