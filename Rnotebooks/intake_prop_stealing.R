#This script wants to find the optimal proportion of time 
#spent stealing during each generation, considering the intake
#rate of every agent.

x <- list()
g <- seq(0,500,10)
for (i in 1:length(g)){
  x[i] <- generation(g[i])
}
str(x)

y <- list()
for (i in 1:length(x)){
  y[[i]] <- tibble(
    id = c(1:length(x[[i]]$fit)),
    ft = as.vector(x[[i]]$foa),
    ht = as.vector(x[[i]]$han),
    st = as.vector(100-(ft+ht)),
    energy = as.vector(x[[i]]$fit)
  )
}
str(y)
 
#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#Concentrating on a single one to test
a <- y[[8]]
a2 <- a %>% 
  mutate(intake_rate=energy/100, p_steal=st/100) %>% 
  select(id, intake_rate, p_steal) %>% 
  arrange(desc(intake_rate))

ggplot(a2, aes(x=p_steal, y=intake_rate))+
  geom_jitter()

a3 <- a2 %>% 
  group_by(intake_rate) %>% 
  summarize(count = n(), mean_psteal = mean(p_steal))
  #summarize(m_int_rate= mean(intake_rate), sd_int_rate= sd(intake_rate))

ggplot(a3, aes(x = mean_psteal, y= intake_rate))+
  geom_point(aes(size=count), alpha=1/3)+
  geom_smooth(se=FALSE)

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#real deal
fun_1 <- function(x){mutate(x,intake_rate=energy/100, p_steal=st/100) %>%
    select(id, intake_rate, p_steal) %>% 
    mutate(p_stl_bin= plyr::round_any(p_steal,0.05)) %>% 
    count(p_stl_bin)}
    #group_by(p_stl_bin) %>%
    #summarise_at(vars(intake_rate), list(~mean(.), ~sd(.)))}

fun_3 <- function(x){mutate(x,intake_rate=energy/100, p_steal=st/100) %>%
    select(id, intake_rate, p_steal) %>%
    arrange(desc(intake_rate))}

fun_2 <- function(x){mutate(x,intake_rate=energy/100, p_steal=st/100) %>% 
    select(id, intake_rate, p_steal) %>% 
    arrange(desc(intake_rate)) %>%
    group_by(intake_rate) %>% 
    summarize(count = n(), mean_psteal = mean(p_steal))}

w <- map(y,fun_1)

ww <- map2_df(w,g, function(a,b){mutate(a, gen = b)})
#nice!

ggplot(ww, aes(x=p_stl_bin))+
  geom_col(aes(y=n))+
  facet_wrap(vars(gen))

ggplot(ww, aes(x=p_stl_bin, y=mean))+
  geom_line()+
  geom_pointrange(aes(ymin=mean-sd, ymax=mean+sd))+
  facet_wrap(vars(gen))

