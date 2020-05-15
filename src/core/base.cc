#include "bash.h"

using workerman::Coroutine;

wmGlobal_t WorkerG;

void workerman_base_init() {
	//初始化timer
	long now_time;
	wmGetMilliTime(&now_time);
	timerwheel_init(&WorkerG.timer, 1, now_time);
	WorkerG.is_running = false;
	WorkerG.poll = NULL;
}

//初始化epoll
int init_wmPoll() {
	size_t size;
	WorkerG.poll = (wmPoll_t *) malloc(sizeof(wmPoll_t));
	if (WorkerG.poll == NULL) {
		wmWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
		return -1;
	}
	WorkerG.poll->epollfd = epoll_create(512); //创建一个epollfd，然后保存在全局变量
	if (WorkerG.poll->epollfd < 0) {
		wmWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
		free(WorkerG.poll);
		WorkerG.poll = NULL;
		return -1;
	}
	WorkerG.poll->ncap = WM_MAXEVENTS;
	size = sizeof(struct epoll_event) * WorkerG.poll->ncap;
	WorkerG.poll->events = (struct epoll_event *) malloc(size);
	memset(WorkerG.poll->events, 0, size);
	WorkerG.poll->event_num = 0; // 事件的数量
	return 0;
}

int free_wmPoll() {
	if (close(WorkerG.poll->epollfd) < 0) {
		wmWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
	}
	free(WorkerG.poll->events);
	WorkerG.poll->events = NULL;
	free(WorkerG.poll);
	WorkerG.poll = NULL;
	return 0;
}

/**
 * 初始化事件
 */
int wm_event_init() {
	if (!WorkerG.poll) {
		init_wmPoll();
	}
	WorkerG.is_running = true;
	return 0;
}

/**
 * 释放整个事件
 */
int wm_event_free() {
	WorkerG.is_running = false;
	free_wmPoll();
	return 0;
}

//调度器
int wm_event_wait() {
	wm_event_init();
	if (!WorkerG.poll) {
		wmError("Need to call wm_event_init first.");
	}

	long mic_time;
	//这里应该改成死循环了
	while (WorkerG.is_running) {
		int n;
		//毫秒级定时器，必须是1
		int timeout = 1;
		epoll_event *events;
		events = WorkerG.poll->events;
		n = epoll_wait(WorkerG.poll->epollfd, events, WorkerG.poll->ncap,
				timeout);
		//循环处理epoll请求
		for (int i = 0; i < n; i++) {
			int fd;
			int id;
			struct epoll_event *p = &events[i];
			//if(p->events & EPOLLIN)

			uint64_t u64 = p->data.u64;
			Coroutine *co;
			//解析出来fd和id
			fromuint64(u64, &fd, &id);
			co = Coroutine::get_by_cid(id);
			co->resume();
		}
		//有定时器才更新
		if (WorkerG.timer.num > 0) {
			//获取毫秒
			wmGetMilliTime(&mic_time);
			timerwheel_update(&WorkerG.timer, mic_time);
		} else if (WorkerG.poll->event_num == 0) {
			WorkerG.is_running = false;
		}

	}
	wm_event_free();

	return 0;
}

/**
 * 把数字转换成字符串
 */
int wm_itoa(char *buf, long value) {
	long i = 0, j;
	long sign_mask;
	unsigned long nn;

	sign_mask = value >> (sizeof(long) * 8 - 1);
	nn = (value + sign_mask) ^ sign_mask;
	do {
		buf[i++] = nn % 10 + '0';
	} while (nn /= 10);

	buf[i] = '-';
	i += sign_mask & 1;
	buf[i] = '\0';

	int s_len = i;
	char swap;

	for (i = 0, j = s_len - 1; i < j; ++i, --j) {
		swap = buf[i];
		buf[i] = buf[j];
		buf[j] = swap;
	}
	buf[s_len] = 0;
	return s_len;
}

/**
 * 随机
 */
int wm_rand(int min, int max) {
	static int _seed = 0;
	assert(max > min);

	if (_seed == 0) {
		_seed = time(NULL);
		srand(_seed);
	}

	int _rand = rand();
	_rand = min
			+ (int) ((double) ((double) (max) - (min) + 1.0)
					* ((_rand) / ((RAND_MAX) + 1.0)));
	return _rand;
}