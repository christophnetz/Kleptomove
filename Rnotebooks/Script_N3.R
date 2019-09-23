## Script N3 after sourcing the SourceMe file
#This script is for creating some MORE COMPLEX plots, starting from a more extended dataset.
#we use the function "generation()" defined in the sourceMe file to import agent-data from different generations.

#Creating an empty list
y <- list()

#Filling the list
for (i in 1:length(list_gen)){
  y[[i]] <- tibble(
    id = c(1:length(list_gen[[i]]$fit)),
    ft = as.vector(list_gen[[i]]$foa),
    ht = as.vector(list_gen[[i]]$han),
    st = as.vector(100-(ft+ht)),
    energy = as.vector(list_gen[[i]]$fit)
  )
}
#str(y)

#Manipulation of data
#function to calculate the proportion of time spent per each activity: (forage, handle, steal)
step0 <- function(x){mutate(x, p_st=st/100, p_ht=ht/100, p_ft=ft/100) %>%
	select(id, p_st, p_ht, p_ft)}

#creating an adequate number of bins and counting the number of agents in each one
#for the 3 different actions:

#foraging
step1 <- function(x){mutate(x, p_bin= plyr::round_any(p_ft,0.05)) %>% 
    count(p_bin, name = "n_ft")}

#stealing
step2 <- function(x){mutate(x, p_bin= plyr::round_any(p_st,0.05)) %>% 
    count(p_bin, name = "n_st")}

#handling
step3 <- function(x){mutate(x, p_bin= plyr::round_any(p_ht,0.05)) %>% 
    count(p_bin, name = "n_ht")}

#creating list-elements mapping the functions
w <- map(y, step0)
w_ft <- map(w, step1)	#list foragers
w_st <- map(w, step2)	#list stealers
w_ht <- map(w, step3)	#list handlers


## TESTING ##---------------------------

#test <- y[[30]]
#test %>% 
#summarise(a=sum(ft),b=sum(ht), c=sum(st), tot=a+b+c)

#test <- w[[40]]
#test %>% 
#summarise(a=mean(p_ft),b=mean(p_ht), c=mean(p_st), tot=a+b+c)

#test <- w_st[[2]]
#test %>% 
#summarise(a=sum(n_st))

## END TESTING ##-------------------------

#Joining the 3 lists in a single one
w_fsh <- pmap(list(w_st, w_ht,w_ft), function(x,y,z){full_join(x,y) %>% 
    full_join(z)})

#Replacing the missing values with zeros
w_fsh0 <- map(w_fsh, function(x){x %>% replace_na(list(n_st=0,n_ht=0,n_ft=0))})

#Creating a single data_frame out of the list of data_frames
df_wfsh <- map2_df(w_fsh0 ,g, function(a,b){mutate(a, gen = b)})

df_wfsh_expanded <- df_wfsh %>% 
  gather(n_st,n_ft,n_ht, key="activity", value = "n_agent")


#PLOT
#
prop_activity <- ggplot(df_wfsh_expanded)+
  geom_bar(mapping = aes(x=p_bin, y=n_agent, fill=activity), stat="identity")+
  scale_y_continuous(breaks = seq(0,10000,2000), minor_breaks = waiver())+
  scale_x_continuous(labels = scales::percent)+
  scale_fill_viridis_d()+
  facet_wrap(vars(gen))+
  my_theme_facet

prop_activity
