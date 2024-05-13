from flask_script import Manager
from flask_migrate import Migrate, MigrateCommand
from main import app, db

manager = Manager(app)

migrate = Migrate(app, db)

manager.add_command("db", MigrateCommand)

if __name__ == "__main__":
    manager.run()
