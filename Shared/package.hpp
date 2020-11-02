namespace package {
	enum e_action
	{
		login,
		list,
		connect,
		exit,
		available,
		unavailable,
		accept,
		check,
		message
	};

	//TODO: change struct name to: request
	struct info
	{
		/* data */
		int id;
		e_action action;
		char content[256];
		bool ok;
	};
}