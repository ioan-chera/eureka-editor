use std::env;

fn main() 
{
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 
    {
        eprintln!("Usage: {} <filename>", args[0]);
        std::process::exit(1);
    }

    let filename = &args[1];
    // TODO: get file hash
}
