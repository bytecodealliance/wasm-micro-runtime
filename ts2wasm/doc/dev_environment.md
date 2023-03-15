# ts2wasm development environment

A VSCode dev-container is available for this project.

### Enter the container

After openning the project using VSCode, click the Green area at the left-bottom corner and select "reopen in container", it may take some time to build the docker image at the first time.

### Update the container

If any modification is made to the files under `.devcontainer`, then press `crtl + shift + P` to call out the command palette, search and select `Remote-Containers: Rebuild Container`

### User in the container

There is a non-root user named `vscode`, it is mapped to the first non-root user in host system, and this user has been enabled to use `sudo` without password

> Note: if you want to use apt to install other softwares, you may get a timeout error if you are behind a proxy. This is because the sudo environment will clear all the environment variables including the proxy related ones. To solve this problem, simply add one line to `/etc/sudoers` **inside the container** `Defaults        env_keep+="http_proxy ftp_proxy all_proxy https_proxy no_proxy"`
> ``` bash
> vscode@ts2wasm-dev:/ts2wasm$ sudo cat /etc/sudoers
> #
> # This file MUST be edited with the 'visudo' command as root.
> #
> # Please consider adding local content in /etc/sudoers.d/ instead of
> # directly modifying this file.
> #
> # See the man page for details on how to write a sudoers file.
> #
> Defaults        env_reset
> Defaults        mail_badpass
> Defaults        env_keep+="http_proxy ftp_proxy all_proxy https_proxy no_proxy" # Add this line
> Defaults        secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
> ```
