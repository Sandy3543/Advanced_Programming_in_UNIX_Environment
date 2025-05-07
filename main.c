#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define ORANGE  "\033[38;2;255;165;0m"
#define WHITE   "\033[37m"

typedef struct settings_s
{
	long init_timestamp;
	int num_of_philos;
	int time_to_eat;
	int time_to_sleep;
	int time_to_die;
	int num_of_meals;
} settings_t;

typedef struct data_s
{
	int philo_index;
	pthread_mutex_t *right_fork;
	pthread_mutex_t *left_fork;

	pthread_mutex_t *checker_lock;
	long last_meal_timestamp;
	bool is_dead;

	// settings
	settings_t spaghetti;
} data_t;

void	parsing_input(settings_t *settings, int ac, char *av[])
{
	settings->num_of_philos = atoi(av[1]);
	settings->time_to_die = atoi(av[2]);
	settings->time_to_eat = atoi(av[3]);
	settings->time_to_sleep = atoi(av[4]);
	settings->num_of_meals = -1;
	if (ac == 6)
		settings->num_of_meals = atoi(av[5]);
}

long get_timestamp()
{
	struct timeval time;
	gettimeofday(&time, NULL);

	long timestamp = time.tv_sec * 1000 + time.tv_usec / 1000;
	return timestamp;
}



long get_now(long init_timestamp)
{
	long now = get_timestamp();
	return now - init_timestamp;
}

void ft_wait(int milliseconds, data_t *data) // 200
{

	long time = get_timestamp();
	long time_after_wait = time + milliseconds;

	while (get_timestamp() < time_after_wait)
	{
		if (get_timestamp() - data->last_meal_timestamp > data->spaghetti.time_to_die)
			break;
		usleep(1);
	}
}

bool is_dead(data_t *data)
{
	pthread_mutex_lock(data->checker_lock);
	return data->is_dead;
}

void kill_all(data_t *data, int num_of_philos)
{
	int i = 0;
	while (i < num_of_philos)
	{
		data[i].is_dead = true;
		//break;
		i++;
	}
}

void routine_undertaker(data_t *data, int num_of_philos)
{
	while (1)
	{
		int i = 0;
		while (i < num_of_philos)
		{
			pthread_mutex_lock(data[i].checker_lock);
			if (data[i].is_dead)
			{
				kill_all(data, num_of_philos);
				pthread_mutex_unlock(data[i].checker_lock);
				return ;
			}
			pthread_mutex_unlock(data[i].checker_lock);
			i++;
		}
	}
}

void smart_printf(data_t *data, char *color, char *str)
{
	pthread_mutex_lock(data->checker_lock);
	if (data->is_dead)
	{
		pthread_mutex_unlock(data->checker_lock);
		return ;
	}
	printf("%s", color);
	printf("%ld\t", get_now(data->spaghetti.init_timestamp));
	printf("[%d] ", data->philo_index);
	printf("%s\n", str);
	printf(RESET);
	pthread_mutex_unlock(data->checker_lock);
}

void *routine(void *arg)
{
	data_t *data = (data_t *)(arg);

	int i = 0;
	while (!data->is_dead && i != data->spaghetti.num_of_meals)
	{
		// check if dead
		// TODO: move this to routine_undertaker
		pthread_mutex_lock(data->checker_lock);
		if (get_timestamp() - data->last_meal_timestamp > data->spaghetti.time_to_die)
		{
			// printf(RED"%ld \t[%d] is dead!!\n"RESET, get_now(data->spaghetti.init_timestamp), data->philo_index);
			smart_printf(data, RED, "is dead!!");
			data->is_dead = true;
			break;
			//routine_undertaker(data, data->spaghetti.num_of_philos);
		}
		pthread_mutex_unlock(data->checker_lock);


		// eating block of code
		pthread_mutex_lock(data->left_fork);
		pthread_mutex_lock(data->right_fork);

		data->last_meal_timestamp = get_timestamp();
		printf(GREEN"%ld \t[%d] is eating\n"RESET, get_now(data->spaghetti.init_timestamp), data->philo_index);
		i++;
		ft_wait(data->spaghetti.time_to_eat, data);

		pthread_mutex_unlock(data->left_fork);
		pthread_mutex_unlock(data->right_fork);


		// sleeping and thinking block of code
		printf(CYAN"%ld \t[%d] is sleeping\n"RESET, get_now(data->spaghetti.init_timestamp), data->philo_index);
		ft_wait(data->spaghetti.time_to_sleep, data);

		pthread_mutex_lock(data->checker_lock);
		if (get_timestamp() - data->last_meal_timestamp > data->spaghetti.time_to_die)
		{
			printf(RED"%ld \t[%d] is dead!!\n"RESET, get_now(data->spaghetti.init_timestamp), data->philo_index);
			data->is_dead = true;
			break;
			//routine_undertaker(data, data->spaghetti.num_of_philos);
		}
		pthread_mutex_unlock(data->checker_lock);

		printf(YELLOW"%ld \t[%d] is thinking\n"RESET, get_now(data->spaghetti.init_timestamp), data->philo_index);
		// long time = get_timestamp();
		// if(time > get_timestamp() + data->spaghetti.time_to_die)
		// 	kill_all(data, data->spaghetti.num_of_philos);
	}
	pthread_mutex_unlock(data->checker_lock);

	return NULL;
}

int main(int ac, char *av[])
{
	int i;
	
	
	if(ac < 5 || ac > 6)
		return 1;
	
	settings_t spaghetti;
	spaghetti.init_timestamp = get_timestamp();
	parsing_input(&spaghetti, ac, av);

	pthread_t philo[spaghetti.num_of_philos];
	pthread_mutex_t *forks = malloc(sizeof(pthread_mutex_t) * spaghetti.num_of_philos);
	pthread_mutex_t *checker_lock = malloc(sizeof(pthread_mutex_t) * spaghetti.num_of_philos);
	data_t data[spaghetti.num_of_philos];

	i = 0;
	while(i < spaghetti.num_of_philos)
	{
		pthread_mutex_init(forks + i, NULL);
		pthread_mutex_init(checker_lock + i, NULL);
		i++;
	}


	i = 0;
	while(i < spaghetti.num_of_philos)
	{
		data[i].philo_index = i;
		data[i].spaghetti = spaghetti;

		data[i].checker_lock = checker_lock + i;
		data[i].last_meal_timestamp = spaghetti.init_timestamp;
		data[i].is_dead = false;

		data[i].left_fork = forks + i;
		data[i].right_fork = forks + (i + 1) % spaghetti.num_of_philos;

		pthread_create(&philo[i], NULL, routine, &data[i]);
		i++;
	}

	routine_undertaker(data, spaghetti.num_of_philos);
	//return 0;

	i = 0;
	while(i < spaghetti.num_of_philos)
	{
		pthread_detach(philo[i]);
		i++;
	}
	/*while(i < spaghetti.num_of_philos)
	{
		pthread_mutex_destroy(&forks[i]);
		i++;
	}*/
	return 0;
}



/*
to make things run in parallal
	- processes ==> fork
	- threads ===> create_thread


./a.out:
num_of_philos ==> 200
time_to_eat ==> 200
time_to_sleep ==> 100
time_to_die ==> 500
meal_count ==> 7

philo => eat -> sleep -> think

		philo1       philo2         philo3        philo4       philo5
fork1          fork2         fork3         fork4       fork5

====================================================================================

gettimeofday
usleep

Implement eating
Implement sleeping
Implement thinking (just a printf)

[ ]- memset,
[x]- printf,
[x]- malloc,
[x]- free,
[ ]- write,
[x]- usleep,
[x]- gettimeofday,
[x]- pthread_create,
[ ]- pthread_detach,
[x]- pthread_join,
[x]- pthread_mutex_init,
[ ]- pthread_mutex_destroy,
[x]- pthread_mutex_lock,
[x]- pthread_mutex_unlock




[ ] - print "has taken a fork"


mutix on checker and on printf





main (54)
 - init_timestamp
 - parsing inputs
 - declare
 	- threads[] on stack
	- forks[] on heap
	- data[] on stack

 - init the forks (mutex) with pthread_mutex_init

 - init for the data and the create the threads on the same while loop

 - start the undertaker

 - join all the threads


routine_undertaker ===> kill_all


routine
 - get_timestamp to check if the philo is dead
 - eat sleep think
     - get_now
 - ft_wait
*/
